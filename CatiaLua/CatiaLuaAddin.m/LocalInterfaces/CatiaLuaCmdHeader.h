// COPYRIGHT Dassault Systemes 2003
//==============================================================================

#ifndef CatiaLuaCmdHeader_H
#define CatiaLuaCmdHeader_H

// System Framework
#include "CATCommandHeader.h"

class CATFrmEditor;

//------------------------------------------------------------------------------
// Command header
// With ENOVIA LCA integration handing
//------------------------------------------------------------------------------
class CatiaLuaCmdHeader : public CATCommandHeader
{
CATDeclareKindOf;
CATDeclareHeaderResources;

public:
  
	CatiaLuaCmdHeader(const CATString & HeaderID,
				   const CATString & loadName,
				   const CATString & classname,
				   void * argument,
				   int state = CATFrmAvailable);  
	CATCommandHeader *Clone();
	virtual ~CatiaLuaCmdHeader();

protected:

	CatiaLuaCmdHeader(CATCommandHeader *header);

	void UpdateAvailability();
	void AddUIActivatedCallback();
	void RemoveUIActivatedCallback();
	void UIActivated(CATCallbackEvent,
			 void *,
			 CATNotification *,
			 CATSubscriberData,
			 CATCallback);

	CATFrmEditor *_FrmEditor;
	CATCallback   _UIActivatedCallback;
};

#endif
