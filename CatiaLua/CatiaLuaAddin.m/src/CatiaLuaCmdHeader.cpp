//------------------------------------------------------------------------------
// Copyright Dassault Systemes 2003
//------------------------------------------------------------------------------
// Command Header Implementation
//------------------------------------------------------------------------------
#include "CatiaLuaCmdHeader.h"

#include "CATFrmEditor.h"




#include "CATAfrCommandHeaderServices.h"
#include <iostream>
#include <string>



//调用其他模块

CATImplementHeaderResources(CatiaLuaCmdHeader,
							CATCommandHeader,
							CatiaLuaCmdHeader);

CATImplementKindOf(CatiaLuaCmdHeader,
				   Implementation,
				   CATCommandHeader,
				   CATNull);

//===========================================================================
CatiaLuaCmdHeader::CatiaLuaCmdHeader(const CATString &headerID,
													   const CATString &loadName,
													   const CATString &className,
													   void * argument,
													   int state)
													   : CATCommandHeader(headerID, loadName, className, argument, state)
{
	// setup callback
	_FrmEditor = CATFrmEditor::GetCurrentEditor();
	_UIActivatedCallback = NULL;
	AddUIActivatedCallback();
	UpdateAvailability();
}

//===========================================================================
CatiaLuaCmdHeader::CatiaLuaCmdHeader(CATCommandHeader *header)
	: CATCommandHeader(header)
{
	// setup callback
	_FrmEditor = CATFrmEditor::GetCurrentEditor();
	_UIActivatedCallback = NULL;
	AddUIActivatedCallback();
	UpdateAvailability();
}

//===========================================================================
CatiaLuaCmdHeader::~CatiaLuaCmdHeader()
{
	UpdateAvailability();
}

//===========================================================================
CATCommandHeader *CatiaLuaCmdHeader::Clone()
{
	return new CatiaLuaCmdHeader(this);
}

//===========================================================================
void CatiaLuaCmdHeader::AddUIActivatedCallback()
{
	if (NULL == _FrmEditor)
		return;
	// Add our callback for UI activated
	_UIActivatedCallback = AddCallback
		(_FrmEditor,
		CATFrmEditor::UIACTIVEOBJECT_CHANGECOMPLETED(),
		(CATSubscriberMethod) &CatiaLuaCmdHeader::UIActivated);
}

//===========================================================================
void CatiaLuaCmdHeader::RemoveUIActivatedCallback()
{
	// Remove callback
	if (NULL != _FrmEditor && NULL != _UIActivatedCallback ) {
		RemoveCallback(_Editor, _UIActivatedCallback);
		_UIActivatedCallback = NULL;
	}
}

//===========================================================================
// This is called when UI has been activated
void CatiaLuaCmdHeader::UIActivated(CATCallbackEvent,
											 void *,
											 CATNotification *,
											 CATSubscriberData,
											 CATCallback)
{
	// Enable/Disable command
	if (NULL == _FrmEditor)
		return;
	UpdateAvailability();
}

//===========================================================================
// Enable/Disable command depending on document storage mode
//===========================================================================
void CatiaLuaCmdHeader::UpdateAvailability()
{
	if (NULL == _FrmEditor)
		return;

	CATCommandHeader* pHeader = NULL;
	CATAfrGetCommandHeader("RunLua", pHeader);
	if (pHeader != NULL)
		pHeader->BecomeAvailable();
}