#ifndef SQ_VMSTATE_H
#define SQ_VMSTATE_H

#ifdef _WIN32
#pragma once
#endif

class SquirrelStateWriter
{
public:
	SquirrelStateWriter( HSQUIRRELVM pVM, CUtlBuffer *pOutput )
		: m_pVM(pVM), m_pBuffer(pOutput) {}
	~SquirrelStateWriter();

	void BeginWrite( void );

private:
	void WriteObject( SQObjectPtr const &obj );
	void WriteVM( HSQUIRRELVM pVM );
	void WriteArray( SQArray *pArray );
	void WriteTable( SQTable *pTable );
	void WriteClass( SQClass *pClass );
	void WriteInstance( SQInstance *pInstance );
	void WriteGenerator( SQGenerator *pGenerator );
	void WriteClosure( SQClosure *pClosure );
	void WriteNativeClosure( SQNativeClosure *pNativeClosure );
	void WriteString( SQString *pString );
	void WriteUserData( SQUserData *pUserData );
	void WriteUserPointer( SQUserPointer pUserPointer );
	void WriteFuncProto( SQFunctionProto *pFuncProto );
	void WriteWeakRef( SQWeakRef *pWeakRef );

	HSQUIRRELVM m_pVM;
	CUtlBuffer *m_pBuffer;
};


class SquirrelStateReader
{
public:
	SquirrelStateReader( HSQUIRRELVM pVM, CUtlBuffer *pOutput )
		: m_pVM(pVM), m_pBuffer(pOutput) {}
	~SquirrelStateReader();

	void BeginRead( void );

private:
	bool ReadObject( SQObjectPtr *pObj, const char *pszName = NULL );
	SQVM *ReadVM();
	SQTable *ReadTable();
	SQArray *ReadArray();
	SQClass *ReadClass();
	SQInstance *ReadInstance();
	SQGenerator *ReadGenerator();
	SQClosure *ReadClosure();
	SQNativeClosure *ReadNativeClosure();
	SQUserData *ReadUserData();
	SQUserPointer *ReadUserPointer();
	SQFunctionProto *ReadFuncProto();
	SQWeakRef *ReadWeakRef();

	HSQUIRRELVM m_pVM;
	CUtlBuffer *m_pBuffer;
};

#endif