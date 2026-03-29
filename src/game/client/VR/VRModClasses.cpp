#include "cbase.h"
#include "VRModClasses.h"

VRGestureMenu *GestureMenu = NULL;

//*************************************************************************
//  VRCollisionBox
//*************************************************************************

VRCollisionBox::VRCollisionBox(Vector myOrigin, Vector myHalfLength, int myBoxID)
{
	boxID = myBoxID;
	origin = myOrigin;
	halfLength = myHalfLength;

	xMin = myOrigin.x - myHalfLength.x;
	xMax = myOrigin.x + myHalfLength.x;
	yMin = myOrigin.y - myHalfLength.y;
	yMax = myOrigin.y + myHalfLength.y;
	zMin = myOrigin.z - myHalfLength.z;
	zMax = myOrigin.z + myHalfLength.z;
}


bool VRCollisionBox::IsPointInBox(Vector point)
{
	return ( ( (xMin < point.x) && (point.x < xMax) ) && ( (yMin < point.y) && (point.y < yMax) ) && ( (zMin < point.z) && (point.z < zMax) ) );
}


//*************************************************************************
//  VRGestureMenuOption
//*************************************************************************

VRGestureMenuOption::VRGestureMenuOption(VRGestureMenu *pMenu, std::string Material, int i)
{
	Index = i;
	GestureQuadMaterial = Material;
	if ( pMenu )
		pMenu->GetGestureQuadsBounds(&UL, &UR, &LL, &LR, &Origin, &HalfLength, Index);
	OptionCollission = new VRCollisionBox(Origin, Vector(HalfLength, HalfLength, HalfLength), Index);
}

VRGestureMenuOption::~VRGestureMenuOption()
{
	delete OptionCollission;
}

//*************************************************************************
//  VRGestureMenu
//*************************************************************************

VRGestureMenu::VRGestureMenu()
{
	std::string OptionMaterials[11];
	GetXMenuMaterials(OptionMaterials, CurrentXMenu);
	for (int j = 0; j < 11; j++)
	{
		MenuOptions[j] = new VRGestureMenuOption(this, OptionMaterials[j], j);
	}
}

VRGestureMenu::~VRGestureMenu()
{
	for ( int j = 0; j < 11; j++ )
	{
		delete MenuOptions[j];
		MenuOptions[j] = NULL;
	}
}

void VRGestureMenu::GetXMenuMaterials(std::string *MaterialsArray, XMenu MyXMenu)
{
	for ( int i = 0; i < 11; i++ )
		MaterialsArray[i] = AllMenuOptionMaterials[MyXMenu][i];
}

void VRGestureMenu::SwitchMenuType(XMenu XMenuToSet)
{
	CurrentXMenu = XMenuToSet;
}

void VRGestureMenu::SetGestureOrigin(Vector pos)
{
	GestureOrigin = pos;
}

void VRGestureMenu::GetGestureQuadsBounds(Vector *pUL, Vector *pUR, Vector *pLL, Vector *pLR, Vector *Origin, float *HalfLength, int i)
{
	(void)i;
	float hl = 4.0f;
	if ( HalfLength )
		*HalfLength = hl;
	Vector o = GestureOrigin;
	if ( Origin )
		*Origin = o;
	Vector hw( hl, hl, hl );
	if ( pUL ) *pUL = o - hw + Vector( 0, hl, hl );
	if ( pUR ) *pUR = o + hw;
	if ( pLL ) *pLL = o - hw;
	if ( pLR ) *pLR = o + Vector( hl, -hl, hl );
}

int VRGestureMenu::SelectGestureOption(Vector GesturePos)
{
	(void)GesturePos;
	return 0;
}

void VRGestureMenu::RenderGestureQuads()
{
}
