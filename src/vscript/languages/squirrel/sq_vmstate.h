#ifndef SQ_VMSTATE_H
#define SQ_VMSTATE_H

#ifdef _WIN32
#pragma once
#endif

class CSQStateIterator
{
public:
	CSQStateIterator( HSQUIRRELVM pVM )
		: m_pVM( pVM ), m_nTab( 0 ), m_bKey( false ) {
		// Make sure no garbage collection happens during this
		__ObjAddRef( pVM );
	}
	~CSQStateIterator() {
		SQCollectable *t = _ss( m_pVM )->_gc_chain;
		while ( t )
		{
			t->UnMark();
			t = t->_next;
		}
		__ObjRelease( m_pVM );
	}

	virtual void PsuedoKey( char const *pszPsuedoKey )
	{
		for ( int i = 0; i < m_nTab; i++ )
		{
			Msg( "  " );
		}

		Msg( "%s: ", pszPsuedoKey );
		m_bKey = true;
	}

	virtual void Key( SQObjectPtr &key )
	{
		for ( int i = 0; i < m_nTab; i++ )
		{
			Msg( "  " );
		}

		SQObjectPtr res;
		m_pVM->ToString( key, res );
		Msg( "%s: ", _stringval( res ) );
		m_bKey = true;
	}

	virtual void Value( SQObjectPtr &value )
	{
		if ( !m_bKey )
		{
			for ( int i = 0; i < m_nTab; i++ )
			{
				Msg( "  " );
			}
		}

		m_bKey = false;

		SQObjectPtr res;
		m_pVM->ToString( value, res );
		if ( ISREFCOUNTED( sq_type( value ) ) )
			Msg( "%s [0x%X]\n", _stringval( res ), _refcounted( value )->_uiRef );
		else
			Msg( "%s\n", _stringval( res ) );
	}

	virtual bool BeginContained( void )
	{
		if ( m_bKey )
		{
			Msg( "\n" );
		}
		m_bKey = false;

		for ( int i = 0; i < m_nTab; i++ )
		{
			Msg( "  " );
		}
		Msg( "{\n" );

		m_nTab++;
		return true;
	}

	virtual void EndContained( void )
	{
		m_nTab--;
		for ( int i = 0; i < m_nTab; i++ )
		{
			Msg( "  " );
		}
		Msg( "}\n" );
	}

private:
	int m_nTab;
	HSQUIRRELVM m_pVM;
	bool m_bKey;
};

void IterateObject( CSQStateIterator *pIterator, SQUnsignedInteger nType, SQObjectPtr &value );
void IterateObject( CSQStateIterator *pIterator, SQObjectPtr &value, char const *pszName=NULL );
void DumpSquirrelState( HSQUIRRELVM pVM );
void WriteSquirrelState( HSQUIRRELVM pVM, CUtlBuffer *pBuffer );
void ReadSquirrelState( HSQUIRRELVM pVM, CUtlBuffer *pBuffer );

#endif