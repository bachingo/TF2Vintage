"Resource/UI/main_menu/itemselectionpanel.res"
{		
	"CTFWeaponSelectPanel"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"CTFWeaponSelectPanel"
		"xpos"				"0"
		"ypos"				"0"
		"zpos"				"0"
		"wide"				"f0"
		"tall"				"f0"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"0"
		"enabled"			"1"
		"border"			""
	}

	"ScrollButtonUp"
	{
		"ControlName"	"CTFWeaponSelectPanel"
		"fieldName"		"ScrollButtonUp"		
		"xpos"			"0"
		"ypos"			"10"
		"zpos"			"60"		
		"wide"			"140"
		"tall"			"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		//"border"		"MainMenuHighlightBorder"
	}
	
	"ScrollButtonDown"
	{
		"ControlName"	"CTFWeaponSelectPanel"
		"fieldName"		"ScrollButtonDown"		
		"xpos"			"0"
		"ypos"			"110"
		"zpos"			"60"		
		"wide"			"140"
		"tall"			"20"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		//"border"		"MainMenuHighlightBorder"
	}
	
	"weaponbutton"
	{
		"ControlName"	"CTFWeaponSelectPanel"
		"fieldName"		"weaponbutton"		
		"xpos"			"33"
		"ypos"			"140"
		"zpos"			"55"		
		"wide"			"140"
		"tall"			"74"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
		//"border"		"MainMenuHighlightBorder"
		"border"		"GrayDialogBorder"
		"SubButton"
		{
			"font"				"HudFontSmallestBold"
			"border_default"	"AdvRoundedButtonDefault"
			"border_armed"		"AdvRoundedButtonArmed"
			"border_depressed"	"AdvRoundedButtonDepressed"	
		}
	}
	
	"weaponsetpanel"
	{
		"ControlName"	"CTFWeaponSelectPanel"
		"fieldName"		"weaponsetpanel"		
		"xpos"			"c-280"
		"ypos"			"94"
		"zpos"			"40"		
		"wide"			"140"
		"tall"			"242"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		//"border"		"MainMenuHighlightBorder"
		"spacing"		"10"
		"bgcolor_override"	"42 39 37 255"
		"SubButton"
		{
			"font"				"HudFontSmallestBold"
			"border_default"	"AdvRoundedButtonDefault"
			"border_armed"		"AdvRoundedButtonArmed"
			"border_depressed"	"AdvRoundedButtonDepressed"	
		}
	}
	
	"BackButton"
	{
		"ControlName"	"CTFAdvButton"
		"fieldName"		"BackButton"
		"xpos"			"c160"
		"ypos"			"437"
		"zpos"			"50"
		"wide"			"100"
		"tall"			"25"
		"visible"		"1"
		"enabled"		"1"
		"command"		"back"		
		
		"SubButton"
		{
			"labelText" 		"#TF_Back"
			"textAlignment"		"center"
			"font"				"HudFontSmallBold"
			"border_default"	"OldAdvButtonDefault"
			"border_armed"		"OldAdvButtonDefaultArmed"
			"border_depressed"	"OldAdvButtonDefaultArmed"	
		}
	}
}
