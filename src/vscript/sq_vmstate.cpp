#include "ivscript.h"

#include "utlbuffer.h"
#include "utlmap.h"
#include "fmtstr.h"

// Include internal Squirrel headers for serialization
#include "squirrel.h"
#include "sqstate.h"
#include "sqvm.h"
#include "sqobject.h"
#include "sqstring.h"
#include "sqarray.h"
#include "sqtable.h"
#include "squserdata.h"
#include "sqfuncproto.h"
#include "sqclass.h"
#include "sqclosure.h"


#include "sq_vector.h"
#include "sq_vmstate.h"



static CUtlMap<void *, void *> s_Pointers( DefLessFunc(void *) );
template <typename T>
static bool BeginRead( T **ppOld, T **ppNew, CUtlBuffer *pBuffer )
{
	*ppOld = (T *)pBuffer->GetPtr();
	if ( *ppOld )
	{
		int iNew = s_Pointers.Find( *ppOld );
		if ( iNew != s_Pointers.InvalidIndex() )
		{
			*ppNew = (T *)s_Pointers[iNew];
			return false;
		}
	}

	*ppNew = NULL;
	return true;
}
static void MapPtr( void *pOld, void *pNew )
{
	s_Pointers.Insert( pOld, pNew );
}

static bool FindKeyForObject( const SQObjectPtr &table, void *p, SQObjectPtr &key )
{
	for( int i = 0; i < _table( table )->_numofnodes; i++ )
	{
		if ( _userpointer( _table( table )->_nodes[i].val ) == p )
		{
			key = _table( table )->_nodes[i].key;
			return true;
		}

		if ( sq_istable( _table( table )->_nodes[i].val ) )
		{
			if ( FindKeyForObject( _table( table )->_nodes[i].val, p, key ) )
			{
				return true;
			}
		}
	}
	return false;
}

static HSQOBJECT LookupObject( char const *szName, HSQUIRRELVM pVM )
{
	SQObjectPtr pObject = _null_;
	
	sq_pushroottable( pVM );

	sq_pushstring( pVM, szName, -1 );
	if ( sq_get( pVM, -2 ) == SQ_OK )
	{
		sq_getstackobj( pVM, -1, &pObject );
		sq_pop( pVM, 1 );
	}

	sq_pop( pVM, 1 );

	return pObject;
}

static const char *SQTypeToString( SQObjectType sqType )
{
	switch( sqType )
	{
		case OT_FLOAT:
			return "FLOAT";
		case OT_INTEGER:
			return "INTEGER";
		case OT_BOOL:
			return "BOOL";
		case OT_STRING:
			return "STRING";
		case OT_NULL:
			return "NULL";
		case OT_TABLE:
			return "TABLE";
		case OT_ARRAY:
			return "ARRAY";
		case OT_CLOSURE:
			return "CLOSURE";
		case OT_NATIVECLOSURE:
			return "NATIVECLOSURE";
		case OT_USERDATA:
			return "USERDATA";
		case OT_GENERATOR:
			return "GENERATOR";
		case OT_THREAD:
			return "THREAD";
		case OT_USERPOINTER:
			return "USERPOINTER";
		case OT_CLASS:
			return "CLASS";
		case OT_INSTANCE:
			return "INSTANCE";
		case OT_WEAKREF:
			return "WEAKREF";
	}

	return "<unknown>";
}

typedef struct
{
	ScriptClassDesc_t *m_pClassDesc;
	void *m_pInstance;
	SQObjectPtr m_instanceUniqueId;
} ScriptInstance_t;


SquirrelStateWriter::~SquirrelStateWriter()
{
	{
		SQCollectable *t = _ss( m_pVM )->_gc_chain;
		while(t) 
		{
			t->UnMark();
			t = t->_next;
		}
	}

	s_Pointers.Purge();
}

void SquirrelStateWriter::BeginWrite( void )
{
	m_pBuffer->PutInt( OT_THREAD );
	m_pBuffer->PutPtr( m_pVM );

	if ( m_pVM->_uiRef & MARK_FLAG )
		return;
	m_pVM->_uiRef |= MARK_FLAG;

	WriteObject( m_pVM->_roottable );

	m_pBuffer->PutInt( m_pVM->_top );
	m_pBuffer->PutInt( m_pVM->_stackbase );

	m_pBuffer->PutUnsignedInt( m_pVM->_stack.size() );
	for( uint i=0; i < m_pVM->_stack.size(); i++ ) 
		WriteObject( m_pVM->_stack[i] );

	m_pBuffer->PutUnsignedInt( m_pVM->_vargsstack.size() );
	for( uint i=0; i < m_pVM->_vargsstack.size(); i++ ) 
		WriteObject( m_pVM->_vargsstack[i] );
}

void SquirrelStateWriter::WriteObject( SQObjectPtr const &obj )
{
	switch ( sq_type( obj ) )
	{
		case OT_NULL:
		{
			m_pBuffer->PutInt( OT_NULL ); 
			break;
		}
		case OT_INTEGER:
		{
			m_pBuffer->PutInt( OT_INTEGER ); 
			m_pBuffer->PutInt( _integer( obj ) );
			break;
		}
		case OT_FLOAT:
		{
			m_pBuffer->PutInt( OT_FLOAT ); 
			m_pBuffer->PutFloat( _float( obj ) );
			break;
		}
		case OT_BOOL:			
		{
			m_pBuffer->PutInt( OT_BOOL ); 
			m_pBuffer->PutInt( _integer( obj ) );
			break;
		}
		case OT_STRING:			
		{
			WriteString( _string( obj ) );
			break;
		}
		case OT_TABLE:
		{
			WriteTable( _table( obj ) );
			break;
		}
		case OT_ARRAY:
		{
			WriteArray( _array( obj ) );
			break;
		}
		case OT_USERDATA:
		{
			WriteUserData( _userdata( obj ) );
			break;
		}
		case OT_CLOSURE:
		{
			WriteClosure( _closure( obj ) );
			break;
		}
		case OT_NATIVECLOSURE:
		{
			WriteNativeClosure( _nativeclosure( obj ) );
			break;
		}
		case OT_GENERATOR:
		{
			WriteGenerator( _generator( obj ) );
			break;
		}
		case OT_USERPOINTER:
		{
			WriteUserPointer( _userpointer( obj ) );
			break;
		}
		case OT_THREAD:
		{
			WriteVM( _thread( obj ) );
			break;
		}
		case OT_FUNCPROTO:
		{
			WriteFuncProto( _funcproto( obj ) );
			break;
		}
		case OT_CLASS:
		{
			WriteClass( _class( obj ) );
			break;
		}
		case OT_INSTANCE:
		{
			WriteInstance( _instance( obj ) );
			break;
		}
		case OT_WEAKREF:
		{
			WriteWeakRef( _weakref( obj ) );
			break;
		}
		default:
			break;
	}
}

void SquirrelStateWriter::WriteGenerator( SQGenerator *pGenerator )
{
	ExecuteNTimes( 1, Msg( "Save load of generators not well tested. caveat emptor\n" ) );
	WriteObject(pGenerator->_closure);

	m_pBuffer->PutInt( OT_GENERATOR );
	m_pBuffer->PutPtr( pGenerator );

	if ( pGenerator->_uiRef & MARK_FLAG )
		return;
	pGenerator->_uiRef |= MARK_FLAG;

	WriteObject( pGenerator->_closure );

	m_pBuffer->PutUnsignedInt( pGenerator->_stack.size() );
	for( uint i=0; i < pGenerator->_stack.size(); i++ )
		WriteObject( pGenerator->_stack[i] );

	m_pBuffer->PutUnsignedInt( pGenerator->_vargsstack.size() );
	for( uint i=0; i < pGenerator->_vargsstack.size(); i++ )
		WriteObject( pGenerator->_vargsstack[i] );
}

void SquirrelStateWriter::WriteClosure( SQClosure *pClosure )
{
	m_pBuffer->PutInt( OT_CLOSURE );
	m_pBuffer->PutPtr( pClosure );

	if ( pClosure->_uiRef & MARK_FLAG )
		return;
	pClosure->_uiRef |= MARK_FLAG;

	WriteObject( pClosure->_function );

	WriteObject( pClosure->_env );

	m_pBuffer->PutUnsignedInt( pClosure->_outervalues.size() );
	for( uint i=0; i < pClosure->_outervalues.size(); i++ )
		WriteObject( pClosure->_outervalues[i] );

	m_pBuffer->PutUnsignedInt( pClosure->_defaultparams.size() );
	for( uint i=0; i < pClosure->_defaultparams.size(); i++ )
		WriteObject( pClosure->_defaultparams[i] );
}

void SquirrelStateWriter::WriteNativeClosure( SQNativeClosure *pNativeClosure )
{
	m_pBuffer->PutInt( OT_NATIVECLOSURE );
	m_pBuffer->PutPtr( pNativeClosure );

	if ( pNativeClosure->_uiRef & MARK_FLAG )
		return;
	pNativeClosure->_uiRef |= MARK_FLAG;

	WriteObject( pNativeClosure->_name );
}

void SquirrelStateWriter::WriteString( SQString *pString )
{
	m_pBuffer->PutInt( OT_STRING ); 
	m_pBuffer->PutInt( pString->_len );
	m_pBuffer->PutString( pString->_val );	
}

void SquirrelStateWriter::WriteUserData( SQUserData *pUserData )
{
	m_pBuffer->PutInt( OT_USERDATA );
	m_pBuffer->PutPtr( pUserData );

	if ( pUserData->_uiRef & MARK_FLAG )
		return;
	pUserData->_uiRef |= MARK_FLAG;
}

void SquirrelStateWriter::WriteUserPointer( SQUserPointer pUserPointer )
{
	m_pBuffer->PutInt( OT_USERPOINTER );
	m_pBuffer->PutPtr( pUserPointer );
}

static SQInteger SqWriteFunc( SQUserPointer up,SQUserPointer data, SQInteger size )
{
	CUtlBuffer *pBuffer = (CUtlBuffer *)up;
	Assert( pBuffer && pBuffer->IsValid() );

	pBuffer->Put( data, size );
	return size;
}

void SquirrelStateWriter::WriteFuncProto( SQFunctionProto *pFuncProto )
{
	m_pBuffer->PutInt( OT_FUNCPROTO );
	m_pBuffer->PutPtr( pFuncProto );

	if ( s_Pointers.Find( pFuncProto ) != s_Pointers.InvalidIndex() )
		return;
	
	s_Pointers.Insert( pFuncProto, pFuncProto );

	pFuncProto->Save( m_pVM, m_pBuffer, &SqWriteFunc );
}

void SquirrelStateWriter::WriteWeakRef( SQWeakRef *pWeakRef )
{
	m_pBuffer->PutInt( OT_WEAKREF );
	WriteObject( pWeakRef->_obj );
}

void SquirrelStateWriter::WriteVM( HSQUIRRELVM pVM )
{
	m_pBuffer->PutInt( OT_THREAD );
	m_pBuffer->PutPtr( pVM );

	if ( pVM->_uiRef & MARK_FLAG )
		return;
	pVM->_uiRef |= MARK_FLAG;

	WriteObject( pVM->_roottable );

	m_pBuffer->PutInt( pVM->_top );
	m_pBuffer->PutInt( pVM->_stackbase );

	m_pBuffer->PutUnsignedInt( pVM->_stack.size() );
	for( uint i=0; i < pVM->_stack.size(); ++i ) 
		WriteObject( pVM->_stack[i] );

	m_pBuffer->PutUnsignedInt( pVM->_vargsstack.size() );
	for( uint i=0; i < pVM->_vargsstack.size(); ++i ) 
		WriteObject( pVM->_vargsstack[i] );
}

void SquirrelStateWriter::WriteArray( SQArray *pArray )
{
	m_pBuffer->PutInt( OT_ARRAY );
	m_pBuffer->PutPtr( pArray );

	if ( pArray->_uiRef & MARK_FLAG )
		return;
	pArray->_uiRef |= MARK_FLAG;

	m_pBuffer->PutUnsignedInt( pArray->_values.size() );
	for ( uint i=0; i < pArray->_values.size(); ++i )
		WriteObject( pArray->_values[i] );
}

void SquirrelStateWriter::WriteTable( SQTable *pTable )
{
	m_pBuffer->PutInt( OT_TABLE );
	m_pBuffer->PutPtr( pTable );

	if ( pTable->_uiRef & MARK_FLAG )
		return;
	pTable->_uiRef |= MARK_FLAG;

	m_pBuffer->PutInt( pTable->_delegate != NULL );
	if ( pTable->_delegate )
		WriteObject( pTable->_delegate );

	m_pBuffer->PutInt( pTable->_numofnodes );
	for( int i=0; i < pTable->_numofnodes; ++i )
	{
		WriteObject( pTable->_nodes[i].key );
		WriteObject( pTable->_nodes[i].val );
	}
}

void SquirrelStateWriter::WriteClass( SQClass *pClass )
{
	m_pBuffer->PutInt( OT_CLASS );
	m_pBuffer->PutPtr( pClass );

	if ( !pClass || ( pClass->_uiRef & MARK_FLAG ) )
		return;
	pClass->_uiRef |= MARK_FLAG;

	bool isNative = ( pClass->_typetag != NULL );
	if ( !isNative )
	{
		for( uint i=0; i < pClass->_methods.size(); ++i ) 
		{
			if ( sq_isnativeclosure( pClass->_methods[i].val ) )
			{
				isNative = true;
				break;
			}
		}
	}

	m_pBuffer->PutInt( isNative );
	if ( isNative )
	{
		if ( pClass->_typetag )
		{
			if ( pClass->_typetag == VECTOR_TYPE_TAG )
			{
				m_pBuffer->PutString( "Vector" );
			}
			else
			{
				ScriptClassDesc_t *pDescriptor = (ScriptClassDesc_t *)pClass->_typetag;
				m_pBuffer->PutString( pDescriptor->m_pszScriptName );
			}
		}
		else
		{
			SQObjectPtr key;
			if ( FindKeyForObject( m_pVM->_roottable, pClass, key ) )
				m_pBuffer->PutString( _stringval( key ) );
			else
				m_pBuffer->PutString( "" );
		}
	}
	else
	{
		m_pBuffer->PutInt( pClass->_base != NULL );
		if ( pClass->_base )
			WriteObject( pClass->_base );

		WriteObject( pClass->_members );

		WriteObject( pClass->_attributes );

		m_pBuffer->PutUnsignedInt( pClass->_defaultvalues.size() );
		for( uint i=0; i < pClass->_defaultvalues.size(); ++i ) 
		{
			WriteObject( pClass->_defaultvalues[i].val );
			WriteObject( pClass->_defaultvalues[i].attrs );
		}

		m_pBuffer->PutUnsignedInt( pClass->_methods.size() );
		for( uint i=0; i < pClass->_methods.size(); ++i ) 
		{
			WriteObject( pClass->_methods[i].val );
			WriteObject( pClass->_methods[i].attrs );
		}

		m_pBuffer->PutUnsignedInt( pClass->_metamethods.size() );
		for( uint i=0; i < pClass->_metamethods.size(); ++i ) 
			WriteObject( pClass->_metamethods[i] );
	}
}

void SquirrelStateWriter::WriteInstance( SQInstance *pInstance )
{
	m_pBuffer->PutInt( OT_INSTANCE );
	m_pBuffer->PutPtr( pInstance );

	if ( pInstance->_uiRef & MARK_FLAG )
		return;
	pInstance->_uiRef |= MARK_FLAG;
	
	WriteObject( pInstance->_class );

	m_pBuffer->PutUnsignedInt( pInstance->_class->_defaultvalues.size() );
	for ( uint i=0; i < pInstance->_class->_defaultvalues.size(); i++ )
		WriteObject( pInstance->_values[i] );

	m_pBuffer->PutPtr( pInstance->_class->_typetag );

	if ( pInstance->_class->_typetag )
	{
		if ( pInstance->_class->_typetag == VECTOR_TYPE_TAG )
		{
			Vector *pVector = (Vector *)pInstance->_userpointer;
			m_pBuffer->PutFloat( pVector->x );
			m_pBuffer->PutFloat( pVector->y );
			m_pBuffer->PutFloat( pVector->z );
		}
		else
		{
			ScriptInstance_t *pContext = (ScriptInstance_t *)pInstance->_userpointer;
			WriteObject( pContext->m_instanceUniqueId );
			m_pBuffer->PutPtr( pContext->m_pInstance );
		}
	}
	else
	{
		WriteUserPointer( NULL );
	}
}



SquirrelStateReader::~SquirrelStateReader()
{
	s_Pointers.Purge();
	sq_collectgarbage( m_pVM );
}

void SquirrelStateReader::BeginRead( void )
{
	m_pBuffer->GetInt();

	void *pOldVM = m_pBuffer->GetPtr();
	s_Pointers.Insert( pOldVM, m_pVM );

	ReadObject( &m_pVM->_roottable );

	m_pVM->_top = m_pBuffer->GetInt();
	m_pVM->_stackbase = m_pBuffer->GetInt();

	int stackSize = m_pBuffer->GetUnsignedInt();
	m_pVM->_stack.resize( stackSize );
	for( int i=0; i < stackSize; i++ ) 
		ReadObject( &m_pVM->_stack[i] );
	
	stackSize = m_pBuffer->GetUnsignedInt();
	for( int i=0; i < stackSize; i++ )
		ReadObject( &m_pVM->_vargsstack[i] );
}

bool SquirrelStateReader::ReadObject( SQObjectPtr *pObj, const char *pszName )
{
	SQObject object;
	object._type = (SQObjectType)m_pBuffer->GetInt();

	switch ( sq_type( object ) )
	{
		case OT_NULL:
		{
			_userpointer( object ) = 0;
			break;
		}
		case OT_INTEGER:
		{
			_integer( object ) = m_pBuffer->GetInt();
			break;
		}
		case OT_FLOAT:
		{
			_float( object ) = m_pBuffer->GetFloat();
			break;
		}
		case OT_BOOL:			
		{
			_integer( object ) = m_pBuffer->GetInt();
			break;
		}
		case OT_STRING:
		{
			int len = m_pBuffer->GetInt();
			char *pString = (char *)stackalloc( len + 1 );
			m_pBuffer->GetString( pString, len + 1 );
			pString[len] = 0;

			_string( object ) = SQString::Create( _ss( m_pVM ), pString, len );
			break;
		}
		case OT_TABLE:
		{
			_table( object ) = ReadTable();
			break;
		}
		case OT_ARRAY:			
		{
			_array( object ) = ReadArray();
			break;
		}
		case OT_USERDATA:
		{
			_userdata( object ) = ReadUserData();			
			break;
		}
		case OT_CLOSURE:
		{
			_closure( object ) = ReadClosure();
			break;
		}
		case OT_NATIVECLOSURE:	
		{
			_nativeclosure( object ) = ReadNativeClosure();
			break;
		}
		case OT_GENERATOR:
		{
			_generator( object ) = ReadGenerator();
			break;
		}
		case OT_USERPOINTER:
		{
			_userpointer( object ) = ReadUserPointer();
			break;
		}
		case OT_THREAD:			
		{
			_thread( object ) = ReadVM();
			break;
		}
		case OT_FUNCPROTO:
		{
			_funcproto( object ) = ReadFuncProto();
			break;
		}
		case OT_CLASS:			
		{
			_class( object ) = ReadClass();			
			break;
		}
		case OT_INSTANCE:
		{
			_instance( object ) = ReadInstance();
			if ( _instance( object ) == NULL )
			{
				HSQOBJECT existingObject = LookupObject( pszName, m_pVM );
				if ( sq_isinstance( existingObject ) )
					_instance( object ) = _instance( existingObject );	
			}

			break;
		}
		case OT_WEAKREF:		
		{
			_weakref( object ) = ReadWeakRef();
			break;
		}
		default:
		{
			_userpointer( object ) = NULL;
			break;
		}
	}

	if ( _RAW_TYPE( sq_type( object ) ) >= _RT_TABLE )
	{
		if ( _userpointer( object ) == NULL )
		{
			DevMsg( "Failed to restore a Squirrel object of type %s\n", SQTypeToString( sq_type( object ) ) );
			pObj->_type = OT_NULL;
			return false;
		}
	}

	*pObj = object;
	return true;;
}

HSQUIRRELVM SquirrelStateReader::ReadVM()
{
	HSQUIRRELVM pVM = sq_newthread( m_pVM, MIN_STACK_OVERHEAD + 2 );
	m_pVM->Pop();
	return pVM;
}

SQTable *SquirrelStateReader::ReadTable()
{
	SQTable *pOld, *pTable;
	if ( !::BeginRead( &pOld, &pTable, m_pBuffer ) )
		return pTable;

	pTable = SQTable::Create( _ss( m_pVM ), 0 );

	MapPtr( pOld, pTable );

	if ( m_pBuffer->GetInt() )
	{
		SQObjectPtr obj;
		ReadObject( &obj );
		pTable->SetDelegate( _table( obj ) );
	}
	else
	{
		pTable->_delegate = NULL;
	}

	int nLength = m_pBuffer->GetInt();
	for( int i=0; i < nLength; ++i )
	{
		SQObjectPtr key;
		ReadObject( &key );

		SQObjectPtr value;
		if ( !ReadObject( &value, sq_isstring( key ) ? _stringval( key ) : NULL ) )
			DevMsg( "Failed to read Squirrel table entry %s\n", sq_isstring( key ) ? _stringval( key ) : SQTypeToString( sq_type( key ) ) );

		if ( !sq_isnull( key ) )
			pTable->NewSlot( key, value );
	}

	return pTable;
}

SQArray *SquirrelStateReader::ReadArray()
{
	SQArray *pOld, *pArray;
	if ( !::BeginRead( &pOld, &pArray, m_pBuffer ) )
		return pArray;

	pArray = SQArray::Create( _ss( m_pVM ), 0 );

	MapPtr( pOld, pArray );

	int nLength = m_pBuffer->GetInt();
	pArray->Reserve( nLength );

	for( int i=0; i < nLength; ++i )
	{
		SQObjectPtr value;
		ReadObject( &value );
		pArray->Append( value );
	}

	return pArray;
}

SQGenerator *SquirrelStateReader::ReadGenerator()
{
	SQGenerator *pOld, *pGenerator;
	if ( !::BeginRead( &pOld, &pGenerator, m_pBuffer ) )
		return pGenerator;

	SQObjectPtr obj;
	ReadObject( &obj );

	pGenerator = SQGenerator::Create( _ss( m_pVM ), _closure( obj ) );
	MapPtr( pOld, pGenerator );

	uint nLength = m_pBuffer->GetUnsignedInt();
	pGenerator->_stack.resize( nLength );
	for ( uint i=0; i < nLength; ++i ) 
		ReadObject( &pGenerator->_stack[i] );

	nLength = m_pBuffer->GetUnsignedInt();
	pGenerator->_vargsstack.resize( nLength );
	for ( uint i=0; i < nLength; ++i ) 
		ReadObject( &pGenerator->_vargsstack[i] );

	return pGenerator;
}

SQClosure *SquirrelStateReader::ReadClosure()
{
	SQClosure *pOld, *pClosure;
	if ( !::BeginRead( &pOld, &pClosure, m_pBuffer ) )
		return pClosure;

	SQObjectPtr obj;
	ReadObject( &obj );

	pClosure = SQClosure::Create( _ss( m_pVM ), _funcproto( obj ) );
	MapPtr( pOld, pClosure );

	ReadObject( &pClosure->_env );

	uint nLength = m_pBuffer->GetUnsignedInt();
	pClosure->_outervalues.resize( nLength );
	for ( uint i=0; i < nLength; ++i ) 
		ReadObject( &pClosure->_outervalues[i] );

	nLength = m_pBuffer->GetUnsignedInt();
	pClosure->_defaultparams.resize( nLength );
	for ( uint i=0; i < nLength; ++i ) 
		ReadObject( &pClosure->_defaultparams[i] );

	return pClosure;
}

SQNativeClosure *SquirrelStateReader::ReadNativeClosure()
{
	SQNativeClosure *pOld, *pClosure;
	if ( !::BeginRead( &pOld, &pClosure, m_pBuffer ) )
		return pClosure;

	SQObjectPtr key;
	ReadObject( &key );

	SQObjectPtr value;
	if ( _table( m_pVM->_roottable )->Get( key, value ) )
	{
		if ( !sq_isnativeclosure( value ) )
		{
			MapPtr( pOld, NULL );
			return nullptr;
		}

		MapPtr( pOld, _nativeclosure( value ) );
		return _nativeclosure( value );
	}

	MapPtr( pOld, NULL );
	return nullptr;
}

SQUserData *SquirrelStateReader::ReadUserData()
{
	m_pBuffer->GetPtr();
	return nullptr;
}

SQUserPointer *SquirrelStateReader::ReadUserPointer()
{
	m_pBuffer->GetPtr();
	return nullptr;
}

static SQInteger SqReadFunc(SQUserPointer up, SQUserPointer data, SQInteger size)
{
	CUtlBuffer *pBuffer = (CUtlBuffer *)up;
	pBuffer->Get( data, size );
	return size;
}

SQFunctionProto *SquirrelStateReader::ReadFuncProto()
{
	SQFunctionProto *pOld, *pPrototype;
	if ( !::BeginRead( &pOld, &pPrototype, m_pBuffer ) )
		return pPrototype;

	SQObjectPtr result;
	SQFunctionProto::Load( m_pVM, m_pBuffer, &SqReadFunc, result );
	pPrototype = _funcproto( result );
	
	pPrototype->_uiRef++;
	result.Null();
	pPrototype->_uiRef--;
	
	MapPtr( pOld, pPrototype );

	return pPrototype;
}

SQWeakRef *SquirrelStateReader::ReadWeakRef()
{
	SQObjectPtr obj;
	ReadObject( &obj );
	if ( _refcounted( obj ) == NULL )
		return NULL;

	SQRefCounted *pRefCounted = _refcounted( obj );
	
	pRefCounted->_uiRef++;

	SQWeakRef *pResult = pRefCounted->GetWeakRef( obj._type );
	obj.Null();

	pRefCounted->_uiRef--;

	return pResult;
}

SQClass *SquirrelStateReader::ReadClass()
{
	SQClass *pOld, *pClass;
	if ( !::BeginRead( &pOld, &pClass, m_pBuffer ) )
		return pClass;

	bool isNative = m_pBuffer->GetInt() != 0;
	if ( isNative )
	{
		char *pszName = (char *)stackalloc( 1024 );
		m_pBuffer->GetString( pszName, 1024 );
		pszName[1023] = '\0';

		SQObjectPtr value;
		if ( _table( m_pVM->_roottable )->Get( SQString::Create( _ss( m_pVM ), pszName ), value ) )
		{
			if ( !sq_isclass( value ) )
			{
				MapPtr( pOld, NULL );
				return NULL;
			}

			MapPtr( pOld, _class( value ) );
			return _class( value );
		}
	}
	else
	{
		SQClass *pBase = NULL;
		if ( m_pBuffer->GetInt() )
		{
			SQObjectPtr base;
			ReadObject( &base );
			pBase = _class( base );
		}

		pClass = SQClass::Create( _ss( m_pVM ), pBase );
		MapPtr( pOld, pClass );

		SQObjectPtr members;
		ReadObject( &members );

		pClass->_members->Release();
		pClass->_members = _table( members );
		pClass->_members->_uiRef++;

		ReadObject( &pClass->_attributes );

		uint nLength = m_pBuffer->GetUnsignedInt();
		pClass->_defaultvalues.resize( nLength );
		for ( uint i=0; i < nLength; ++i ) 
		{
			ReadObject( &pClass->_defaultvalues[i].val );
			ReadObject( &pClass->_defaultvalues[i].attrs );
		}

		nLength = m_pBuffer->GetUnsignedInt();
		pClass->_methods.resize( nLength );
		for ( uint i=0; i < nLength; ++i ) 
		{
			ReadObject( &pClass->_methods[i].val );
			ReadObject( &pClass->_methods[i].attrs );
		}

		nLength = m_pBuffer->GetUnsignedInt();
		pClass->_metamethods.resize( nLength );
		for ( uint i=0; i < nLength; ++i ) 
			ReadObject( &pClass->_metamethods[i] );

		return pClass;
	}

	MapPtr( pOld, NULL );
	return NULL;
}

SQInstance *SquirrelStateReader::ReadInstance()
{
	SQInstance *pOld, *pInstance;
	if ( !::BeginRead( &pOld, &pInstance, m_pBuffer ) )
		return pInstance;

	SQObjectPtr obj;
	ReadObject( &obj );

	// Error! just consume the buffer data
	if ( _class( obj ) == NULL )
	{
		MapPtr( pOld, NULL );

		int nLength = m_pBuffer->GetUnsignedInt();
		for ( int i=0; i < nLength; ++i ) 
		{
			SQObjectPtr unused;
			ReadObject( &unused );
		}

		void *pTypeTag = m_pBuffer->GetPtr();
		if ( pTypeTag )
		{
			if ( pTypeTag == VECTOR_TYPE_TAG )
			{
				m_pBuffer->GetFloat();
				m_pBuffer->GetFloat();
				m_pBuffer->GetFloat();
			}
			else
			{
				SQObjectPtr unused;
				ReadObject( &unused );

				m_pBuffer->GetPtr();
			}
		}
		else
		{
			m_pBuffer->GetInt();
			ReadUserPointer();
		}

		return NULL;
	}
	else
	{
		pInstance = SQInstance::Create( _ss( m_pVM ), _class( obj ) );

		int nLength = m_pBuffer->GetUnsignedInt();
		for ( int i=0; i < nLength; ++i )
			ReadObject( &pInstance->_values[i] );

		// unneccesary, just consume
		m_pBuffer->GetPtr();

		if ( pInstance->_class->_typetag )
		{
			if ( pInstance->_class->_typetag == VECTOR_TYPE_TAG )
			{
				Vector *pValue = new Vector;
				pValue->x = m_pBuffer->GetFloat();
				pValue->y = m_pBuffer->GetFloat();
				pValue->z = m_pBuffer->GetFloat();
				pInstance->_userpointer = pValue;
			}
			else
			{
				ScriptInstance_t *pData = new ScriptInstance_t;
				pData->m_pInstance = NULL;
				pData->m_pClassDesc = (ScriptClassDesc_t *)pInstance->_class->_typetag;
				ReadObject( &pData->m_instanceUniqueId );

				void *pPreviousValue = m_pBuffer->GetPtr();
				if ( sq_isstring( pData->m_instanceUniqueId ) )
				{
					IScriptInstanceHelper *pHelper = pData->m_pClassDesc->pHelper;
					if ( pHelper )
					{
						HSQOBJECT *pHandle = new HSQOBJECT;
						pHandle->_type = OT_INSTANCE;
						pHandle->_unVal.pInstance = pInstance;

						pData->m_pInstance = pHelper->BindOnRead( (HSCRIPT)pHandle, pPreviousValue, _stringval( pData->m_instanceUniqueId ) );
						if ( pData->m_pInstance )
						{
							pInstance->_uiRef++;
							sq_addref( m_pVM, pHandle );
							pInstance->_uiRef--;
						}
						else
						{
							delete pHandle;
						}
					}

					if ( pData->m_pInstance == NULL )
					{
						HSQOBJECT existingObject = LookupObject( _stringval( pData->m_instanceUniqueId ), m_pVM );
						if ( !sq_isnull( existingObject ) )
						{
							if ( sq_isinstance( existingObject ) && _class( existingObject ) == pInstance->_class )
							{
								delete pInstance;
								return _instance( existingObject );
							}
						}
					}
				}

				pInstance->_userpointer = pData;
			}
		}
		else
		{
			m_pBuffer->GetInt();
			pInstance->_userpointer = ReadUserPointer();
		}

		MapPtr( pOld, pInstance );
	}

	return pInstance;
}
