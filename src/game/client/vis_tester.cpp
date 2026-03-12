#include "cbase.h"

#include "filesystem.h"
#include "lumpfiles.h"
#include "tier1/memstack.h"
#include "lzmaDecoder.h"

#include "tier0/memdbgon.h"

#pragma region CORE
static CMemoryStack g_MemStack;
template <typename T> static T* Hunk_Alloc( size_t s, bool clear = true )
{
	return static_cast<T*>( g_MemStack.Alloc( s, clear ) );
}

static void CM_LoadMap( const char* name );
static void CM_FreeMap();
static class CVisMemoryManager : public CAutoGameSystem
{
public:
	CVisMemoryManager() : CAutoGameSystem( "visMemoryManager" ) {}

	bool Init() OVERRIDE
	{
		const int nMaxBytes = 48 * 1024 * 1024;
		const int nMinCommitBytes = 0x8000;
		const int nInitialCommit = 0x280000;
		g_MemStack.Init( nMaxBytes, nMinCommitBytes, nInitialCommit );
		return true;
	}

	void Shutdown() OVERRIDE
	{
		g_MemStack.FreeAll();
	}

	void LevelInitPreEntity() OVERRIDE
	{
		CM_LoadMap( VarArgs( "maps/%s.bsp", MapName() ) );
	}

	void LevelShutdownPreEntity() OVERRIDE
	{
		CM_FreeMap();
	}

} memManager;

template <class T>
class CRangeValidatedArray
{
public:
	void Attach( int nCount, T* pData )
	{
		m_pArray = pData;

#ifdef _DEBUG
		m_nCount = nCount;
#endif
	}

	void Detach()
	{
		m_pArray = NULL;

#ifdef _DEBUG
		m_nCount = 0;
#endif
	}

	T &operator[]( int i )
	{
		Assert( ( i >= 0 ) && ( i < m_nCount ) );
		return m_pArray[i];
	}

	const T &operator[]( int i ) const
	{
		Assert( ( i >= 0 ) && ( i < m_nCount ) );
		return m_pArray[i];
	}

	T* Base()
	{
		return m_pArray;
	}

private:
	T* m_pArray;

#ifdef _DEBUG
	int m_nCount;
#endif
};

struct cnode_t
{
	cplane_t*	plane;
	int			children[2];		// negative numbers are leafs
};
struct cleaf_t
{
	int			    contents;
	short			cluster;
	short			area : 9;
	short			flags : 7;
	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
	unsigned short	dispListStart;
	unsigned short	dispCount;
};

class CCollisionBSPData
{
public:
	// This is sort of a hack, but it was a little too painful to do this any other way
	// The goal of this dude is to allow us to override the tree with some
	// other tree (or a subtree)
	cnode_t*							map_rootnode;

	int									numplanes;
	CRangeValidatedArray<cplane_t>		map_planes;
	int									numnodes;
	CRangeValidatedArray<cnode_t>		map_nodes;
	int									numleafs;				// allow leaf funcs to be called without a map
	CRangeValidatedArray<cleaf_t>		map_leafs;
	int									emptyleaf;

	// this points to the whole block of memory for vis data, but it is used to
	// reference the header at the top of the block.
	int									numvisibility;
	dvis_t*								map_vis;

	int									numclusters;
};

class CMapLoadHelper
{
public:
	CMapLoadHelper( int lumpToLoad )
	{
		if ( s_MapFileHandle == FILESYSTEM_INVALID_HANDLE )
		{
			Warning( "Can't load map from invalid handle!!!" );
			return;
		}
		
		if ( lumpToLoad < 0 || lumpToLoad >= HEADER_LUMPS )
		{
			Warning( "Can't load lump %i, range is 0 to %i!!!", lumpToLoad, HEADER_LUMPS - 1 );
		}
	
		m_nLumpID = lumpToLoad;
		m_pData = NULL;
		m_pRawData = NULL;
	
		// Load raw lump from disk
		lump_t* lump = &s_MapHeader.lumps[lumpToLoad];

		m_nLumpSize = lump->filelen;
		m_nLumpUncompressedSize = lump->uncompressedSize;
		m_nLumpOffset = lump->fileofs;
		m_nLumpVersion = lump->version;	

		if ( !m_nLumpSize )
			return;	// this lump has no data

		unsigned nOffsetAlign, nSizeAlign, nBufferAlign;
		filesystem->GetOptimalIOConstraints( s_MapFileHandle, &nOffsetAlign, &nSizeAlign, &nBufferAlign );

		const bool bTryOptimal = ( m_nLumpOffset % 4 == 0 ); // Don't return badly aligned data
		unsigned int alignedOffset = m_nLumpOffset;
		unsigned int alignedBytesToRead = ( ( m_nLumpSize ) ? m_nLumpSize : 1 );

		if ( bTryOptimal )
		{
			alignedOffset = AlignValue( ( alignedOffset - nOffsetAlign ) + 1, nOffsetAlign );
			alignedBytesToRead = AlignValue( ( m_nLumpOffset - alignedOffset ) + alignedBytesToRead, nSizeAlign );
		}

		m_pRawData = static_cast<byte*>( filesystem->AllocOptimalReadBuffer( s_MapFileHandle, alignedBytesToRead, alignedOffset ) );
		if ( !m_pRawData )
		{
			Warning( "Can't load lump %i, allocation of %i bytes failed!!!", lumpToLoad, m_nLumpSize + 1 );
		}

		filesystem->Seek( s_MapFileHandle, alignedOffset, FILESYSTEM_SEEK_HEAD );
		filesystem->ReadEx( m_pRawData, alignedBytesToRead, alignedBytesToRead, s_MapFileHandle );

		if ( m_nLumpUncompressedSize )
		{
			if ( CLZMA::IsCompressed( m_pRawData ) && m_nLumpUncompressedSize == static_cast<int>( CLZMA::GetActualSize( m_pRawData ) ) )
			{
				m_pData = static_cast<byte*>( MemAlloc_AllocAligned( m_nLumpUncompressedSize, 4 ) );
				const int outSize = CLZMA::Uncompress( m_pRawData, m_pData );
				if ( outSize != m_nLumpUncompressedSize )
				{
					Warning( "Decompressed size differs from header, BSP may be corrupt\n" );
				}
			}
			else
			{
				Assert( 0 );
				Warning( "Unsupported BSP: Unrecognized compressed lump\n" );
			}
		}
		else
		{
			m_pData = m_pRawData + ( m_nLumpOffset - alignedOffset );
		}
	}
	
	~CMapLoadHelper()
	{
		if ( m_nLumpUncompressedSize )
		{
			MemAlloc_FreeAligned( m_pData );
		}
		if ( m_pRawData )
		{
			filesystem->FreeOptimalReadBuffer( m_pRawData );
		}
	}

	byte* LumpBase() const
	{
		return m_pData;
	}
	
	int	LumpSize() const
	{
		return m_nLumpUncompressedSize ? m_nLumpUncompressedSize : m_nLumpSize;
	}
	
	int	LumpOffset() const
	{
		return m_nLumpOffset;
	}
	
	int	LumpVersion() const
	{
		return m_nLumpVersion;
	}

	// Global setup/shutdown
	static void	Init( const char* loadname )
	{
		if ( ++s_nMapLoadRecursion > 1 )
		{
			return;
		}

		s_MapFileHandle = FILESYSTEM_INVALID_HANDLE;
		V_memset( &s_MapHeader, 0, sizeof( s_MapHeader ) );

		V_strcpy_safe( s_szLoadName, loadname );
		s_MapFileHandle = filesystem->OpenEx( loadname, "rb" );
		if ( s_MapFileHandle == FILESYSTEM_INVALID_HANDLE )
		{
			Warning( "CMapLoadHelper::Init, unable to open %s\n", loadname );
			return;
		}

		filesystem->Read( &s_MapHeader, sizeof( dheader_t ), s_MapFileHandle );
		if ( s_MapHeader.ident != IDBSPHEADER )
		{
			filesystem->Close( s_MapFileHandle );
			s_MapFileHandle = FILESYSTEM_INVALID_HANDLE;
			Warning( "CMapLoadHelper::Init, map %s has wrong identifier\n", loadname );
			return;
		}

		if ( s_MapHeader.version < MINBSPVERSION || s_MapHeader.version > BSPVERSION )
		{
			filesystem->Close( s_MapFileHandle );
			s_MapFileHandle = FILESYSTEM_INVALID_HANDLE;
			Warning( "CMapLoadHelper::Init, map %s has wrong version (%i when expecting %i)\n", loadname,
				s_MapHeader.version, BSPVERSION );
		}
	}

	static void	Shutdown()
	{
		if ( --s_nMapLoadRecursion > 0 )
		{
			return;
		}

		if ( s_MapFileHandle != FILESYSTEM_INVALID_HANDLE )
		{
			filesystem->Close( s_MapFileHandle );
			s_MapFileHandle = FILESYSTEM_INVALID_HANDLE;
		}

		s_szLoadName[ 0 ] = 0;
		V_memset( &s_MapHeader, 0, sizeof( s_MapHeader ) );
	}

private:
	int					m_nLumpSize;
	int					m_nLumpUncompressedSize;
	int					m_nLumpOffset;
	int					m_nLumpVersion;
	byte*				m_pRawData;
	byte*				m_pData;

	// Handling for lump files
	int					m_nLumpID;
	
	static dheader_t		s_MapHeader;
	static FileHandle_t		s_MapFileHandle;
	static char				s_szLoadName[64];
	static int				s_nMapLoadRecursion;
};
dheader_t CMapLoadHelper::s_MapHeader;
FileHandle_t CMapLoadHelper::s_MapFileHandle = FILESYSTEM_INVALID_HANDLE;
char CMapLoadHelper::s_szLoadName[64];
int CMapLoadHelper::s_nMapLoadRecursion = 0;

static void CollisionBSPData_LoadLeafs( CCollisionBSPData* );
static void CollisionBSPData_LoadVisibility( CCollisionBSPData* );
static void CollisionBSPData_LoadNodes( CCollisionBSPData* );
static void CollisionBSPData_LoadPlanes( CCollisionBSPData* );
static void CollisionBSPData_Load( CCollisionBSPData* pBSPData )
{
	COM_TimestampedLog( "  CollisionBSPData_LoadPlanes" );
	CollisionBSPData_LoadPlanes( pBSPData );

	COM_TimestampedLog( "  CollisionBSPData_LoadNodes" );
	CollisionBSPData_LoadNodes( pBSPData );

	COM_TimestampedLog( "  CollisionBSPData_LoadLeafs" );
	CollisionBSPData_LoadLeafs( pBSPData );

	COM_TimestampedLog( "  CollisionBSPData_LoadVisibility" );
	CollisionBSPData_LoadVisibility( pBSPData );
}

static void CollisionBSPData_LoadLeafs_Version_0( CCollisionBSPData* pBSPData, CMapLoadHelper &lh )
{
	dleaf_version_0_t* in = reinterpret_cast<dleaf_version_0_t*>( lh.LumpBase() );
	if ( lh.LumpSize() % sizeof( dleaf_version_0_t ) )
		Warning( "CollisionBSPData_LoadLeafs: funny lump size");

	const int count = lh.LumpSize() / sizeof( dleaf_version_0_t );

	if (count < 1)
		Warning( "Map with no leafs");

	// need to save space for box planes
	if (count > MAX_MAP_PLANES)
		Warning( "Map has too many planes");

	// Need an extra one for the emptyleaf below
	const int nSize = ( count + 1 ) * sizeof( cleaf_t );
	pBSPData->map_leafs.Attach( count + 1, Hunk_Alloc<cleaf_t>( nSize ) );

	pBSPData->numleafs = count;
	pBSPData->numclusters = 0;

	for ( int i = 0 ; i<count ; i++, in++ )
	{
		cleaf_t	*out = &pBSPData->map_leafs[i];	
		out->contents = in->contents;
		out->cluster = in->cluster;
		out->area = in->area;
		out->flags = in->flags;
		out->firstleafbrush = in->firstleafbrush;
		out->numleafbrushes = in->numleafbrushes;

		out->dispCount = 0;

		if (out->cluster >= pBSPData->numclusters)
		{
			pBSPData->numclusters = out->cluster + 1;
		}
	}

	if (pBSPData->map_leafs[0].contents != CONTENTS_SOLID)
		Warning( "Map leaf 0 is not CONTENTS_SOLID");

	pBSPData->emptyleaf = pBSPData->numleafs;
	V_memset( &pBSPData->map_leafs[pBSPData->emptyleaf], 0, sizeof(pBSPData->map_leafs[pBSPData->emptyleaf]) );
	pBSPData->numleafs++;
}

static void CollisionBSPData_LoadLeafs_Version_1( CCollisionBSPData* pBSPData, CMapLoadHelper &lh )
{
	dleaf_t* in = reinterpret_cast<dleaf_t*>( lh.LumpBase() );
	if ( lh.LumpSize() % sizeof( dleaf_t ) )
		Warning( "CollisionBSPData_LoadLeafs: funny lump size");

	const int count = lh.LumpSize() / sizeof( dleaf_t );

	if (count < 1)
		Warning( "Map with no leafs");

	// need to save space for box planes
	if (count > MAX_MAP_PLANES)
		Warning( "Map has too many planes");

	// Need an extra one for the emptyleaf below
	const int nSize = ( count + 1 ) * sizeof( cleaf_t );
	pBSPData->map_leafs.Attach( count + 1, Hunk_Alloc<cleaf_t>( nSize ) );

	pBSPData->numleafs = count;
	pBSPData->numclusters = 0;

	for ( int i = 0 ; i<count ; i++, in++ )
	{
		cleaf_t	*out = &pBSPData->map_leafs[i];	
		out->contents = in->contents;
		out->cluster = in->cluster;
		out->area = in->area;
		out->flags = in->flags;
		out->firstleafbrush = in->firstleafbrush;
		out->numleafbrushes = in->numleafbrushes;

		out->dispCount = 0;

		if (out->cluster >= pBSPData->numclusters)
		{
			pBSPData->numclusters = out->cluster + 1;
		}
	}

	if (pBSPData->map_leafs[0].contents != CONTENTS_SOLID)
		Warning( "Map leaf 0 is not CONTENTS_SOLID");

	pBSPData->emptyleaf = pBSPData->numleafs;
	V_memset( &pBSPData->map_leafs[pBSPData->emptyleaf], 0, sizeof(pBSPData->map_leafs[pBSPData->emptyleaf]) );
	pBSPData->numleafs++;
}

static void CollisionBSPData_LoadLeafs( CCollisionBSPData* pBSPData )
{
	CMapLoadHelper lh( LUMP_LEAFS );
	switch( lh.LumpVersion() )
	{
	case 0:
		CollisionBSPData_LoadLeafs_Version_0( pBSPData, lh );
		break;
	case 1:
		CollisionBSPData_LoadLeafs_Version_1( pBSPData, lh );
		break;
	default:
		Assert( 0 );
		Warning( "Unknown LUMP_LEAFS version\n" );
		break;
	}
}

static void CollisionBSPData_LoadVisibility( CCollisionBSPData* pBSPData )
{
	CMapLoadHelper lh( LUMP_VISIBILITY );

	pBSPData->numvisibility = lh.LumpSize();
	if ( lh.LumpSize() > MAX_MAP_VISIBILITY )
		Warning( "Map has too large visibility lump" );

	const int visDataSize = lh.LumpSize();
	if ( visDataSize == 0 )
	{
		pBSPData->map_vis = NULL;
	}
	else
	{
		pBSPData->map_vis = Hunk_Alloc<dvis_t>( visDataSize, false );
		V_memcpy( pBSPData->map_vis, lh.LumpBase(), visDataSize );
	}
}

static void CollisionBSPData_LoadNodes( CCollisionBSPData* pBSPData )
{
	CMapLoadHelper lh( LUMP_NODES );

	dnode_t* in = reinterpret_cast<dnode_t*>( lh.LumpBase() );
	if ( lh.LumpSize() % sizeof( dnode_t ) )
		Warning( "CollisionBSPData_LoadNodes: funny lump size" );
	const int count = lh.LumpSize() / sizeof( dnode_t );

	if ( count < 1 )
		Warning( "Map has no nodes" );

	if ( count > MAX_MAP_NODES )
		Warning( "Map has too many nodes" );

	// 6 extra for box hull
	const int nSize = ( count + 6 ) * sizeof( cnode_t );
	pBSPData->map_nodes.Attach( count + 6, Hunk_Alloc<cnode_t>( nSize ) );

	pBSPData->numnodes = count;
	pBSPData->map_rootnode = pBSPData->map_nodes.Base();

	for ( int i = 0; i<count; i++, in++ )
	{
		cnode_t	*out = &pBSPData->map_nodes[i];
		out->plane = &pBSPData->map_planes[in->planenum];
		for ( int j = 0; j<2; j++ )
		{
			out->children[j] = in->children[j];
		}
	}
}

static void CollisionBSPData_LoadPlanes( CCollisionBSPData* pBSPData )
{
	CMapLoadHelper lh( LUMP_PLANES );

	dplane_t* in = reinterpret_cast<dplane_t*>( lh.LumpBase() );
	if ( lh.LumpSize() % sizeof( dplane_t ) )
		Warning( "CollisionBSPData_LoadPlanes: funny lump size" );

	const int count = lh.LumpSize() / sizeof( dplane_t );

	if ( count < 1 )
		Warning( "Map with no planes" );

	// need to save space for box planes
	if ( count > MAX_MAP_PLANES )
		Warning( "Map has too many planes" );

	const int nSize = count * sizeof( cplane_t );
	pBSPData->map_planes.Attach( count, Hunk_Alloc<cplane_t>( nSize ) );
	pBSPData->numplanes = count;

	for ( int i = 0; i<count; i++, in++ )
	{
		cplane_t* out = &pBSPData->map_planes[i];
		int bits = 0;
		for ( int j = 0; j<3; j++ )
		{
			out->normal[j] = in->normal[j];
			if ( out->normal[j] < 0 )
			{
				bits |= 1 << j;
			}
		}

		out->dist = in->dist;
		out->type = in->type;
		out->signbits = bits;
	}
}

static void CollisionBSPData_Init( CCollisionBSPData* pBSPData )
{
	pBSPData->numleafs = 1;
	pBSPData->map_vis = NULL;
	pBSPData->numclusters = 1;
}

static CCollisionBSPData* GetCollisionBSPData()
{
	static CCollisionBSPData data;
	return &data;
}

static void CM_FreeMap()
{
	// get the current collision bsp -- there is only one!
	CCollisionBSPData* pBSPData = GetCollisionBSPData();

	if ( pBSPData->map_planes.Base() )
	{
		pBSPData->map_planes.Detach();
	}

	if ( pBSPData->map_leafs.Base() )
	{
		pBSPData->map_leafs.Detach();
	}

	if ( pBSPData->map_nodes.Base() )
	{
		pBSPData->map_nodes.Detach();
	}

	if ( pBSPData->map_vis )
	{
		pBSPData->map_vis = NULL;
	}

	pBSPData->numplanes = 0;
	pBSPData->emptyleaf = 0;
	pBSPData->numnodes = 0;
	pBSPData->numleafs = 0;
	pBSPData->numclusters = 0;
	pBSPData->numvisibility = 0;
	pBSPData->map_rootnode = NULL;
}

static void CM_LoadMap( const char* name )
{
	// get the current bsp -- there is currently only one!
	CCollisionBSPData* pBSPData = GetCollisionBSPData();
	
	// initialize the collision bsp data
	CollisionBSPData_Init( pBSPData );

	if ( !name || !name[0] )
		return;

	// read in the collision model data
	CMapLoadHelper::Init( name );
	CollisionBSPData_Load( pBSPData );
	CMapLoadHelper::Shutdown( );
}

static int	CM_NumClusters()
{
	return GetCollisionBSPData()->numclusters;
}

static void CM_NullVis( CCollisionBSPData* pBSPData, byte* out )
{
	int numClusterBytes = ( pBSPData->numclusters + 7 ) >> 3;
	byte* out_p = out;

	while (numClusterBytes)
	{
		*out_p++ = 0xff;
		numClusterBytes--;
	}
}

static void CM_DecompressVis( CCollisionBSPData* pBSPData, int cluster, int visType, byte* out )
{
	if ( !pBSPData )
	{
		Assert( false ); // Shouldn't ever happen.
	}

	if ( cluster > pBSPData->numclusters || cluster < 0 )
	{
		// This can happen if this is called while the level is loading. See Map_VisCurrentCluster.
		CM_NullVis( pBSPData, out );
		return;
	}

	// no vis info, so make all visible
	if ( !pBSPData->numvisibility || !pBSPData->map_vis )
	{	
		CM_NullVis( pBSPData, out );
		return;		
	}

	byte* in = reinterpret_cast<byte*>( pBSPData->map_vis ) + pBSPData->map_vis->bitofs[cluster][visType];
	const int numClusterBytes = ( pBSPData->numclusters + 7 ) >> 3;
	byte* out_p = out;

	// no vis info, so make all visible
	if ( !in )
	{	
		CM_NullVis( pBSPData, out );
		return;		
	}

	do
	{
		if (*in)
		{
			*out_p++ =* in++;
			continue;
		}

		int c = in[1];
		in += 2;
		if ((out_p - out) + c > numClusterBytes)
		{
			c = numClusterBytes - (out_p - out);
			DevWarning( "Vis decompression overrun\n" );
		}
		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < numClusterBytes);
}

static void CM_Vis( byte* dest, int destlen, int cluster, int visType )
{
	// get the current collision bsp -- there is only one!
	CCollisionBSPData* pBSPData = GetCollisionBSPData();

	if ( !dest || visType > 2 || visType < 0 )
	{
		Warning( "CM_Vis: error");
		return;
	}

	if ( cluster == -1 )
	{
		int len = (pBSPData->numclusters+7)>>3;
		if ( len > destlen )
		{
			Warning( "CM_Vis:  buffer not big enough (%i but need %i)\n",
				destlen, len );
		}
		V_memset( dest, 0, (pBSPData->numclusters+7)>>3 );
	}
	else
	{
		CM_DecompressVis( pBSPData, cluster, visType, dest );
	}
}

static int	CM_LeafCluster( int leafnum )
{
	const CCollisionBSPData* pBSPData = GetCollisionBSPData();

	Assert( leafnum >= 0 );
	Assert( leafnum < pBSPData->numleafs );

	return pBSPData->map_leafs[leafnum].cluster;
}

static int CM_PointLeafnum_r( CCollisionBSPData* pBSPData, const Vector& p, int num )
{
	float d;
	while (num >= 0)
	{
		cnode_t* node = pBSPData->map_rootnode + num;
		cplane_t* plane = node->plane;

		if ( plane->type < 3 )
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	return -1 - num;
}

static int CM_PointLeafnum (const Vector& p)
{
	// get the current collision bsp -- there is only one!
	CCollisionBSPData* pBSPData = GetCollisionBSPData();

	if ( !pBSPData->numnodes )
		return 0;

	return CM_PointLeafnum_r(pBSPData, p, 0);
}
#pragma endregion

int GetClusterForOrigin( const Vector& org )
{
	return CM_LeafCluster( CM_PointLeafnum( org ) );
}

int GetPVSForCluster( int clusterIndex, int outputpvslength, byte* outputpvs )
{
	int length = (CM_NumClusters()+7)>>3;

	if ( outputpvs )
	{
		if ( outputpvslength < length )
		{
			Warning( "GetPVSForOrigin called with inusfficient sized pvs array, need %i bytes!", length );
			return length;
		}
		
		CM_Vis( outputpvs, outputpvslength, clusterIndex, DVIS_PVS );
	}

	return length;
}

bool CheckOriginInPVS( const Vector& org, const unsigned char *checkpvs, int checkpvssize )
{
	int clusterIndex = GetClusterForOrigin( org );
	
	if ( clusterIndex < 0 )
		return false;
		
	int offset = clusterIndex>>3;
	if ( offset > checkpvssize )
	{
		Warning( "CheckOriginInPVS:  cluster would read past end of pvs data (%i:%i)\n",
			offset, checkpvssize );
		return false;
	}
		
	if ( !(checkpvs[offset] & (1<<(clusterIndex&7)) ) )
	{
		return false;
	}
		
	return true;
}