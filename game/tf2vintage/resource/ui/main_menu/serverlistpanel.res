"Resource/UI/main_menu/ServerlistPanel.res"
{	
	"ServerlistPanel"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"ServerlistPanel"
		"xpos"				"25"
		"ypos"				"375"
		"zpos"				"3"
		"wide"				"1"
		"tall"				"1"
		"visible"			"1"
		"enabled"			"0"
	}
	
	"WelcomeBG"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"WelcomeBG"
		"xpos"				"5"
		"ypos"				"5"
		"zpos"				"4"
		"wide"				"1"
		"tall"				"1"
		"visible"			"1"
		"enabled"			"0"
		"border"			"MainMenuBGBorderAlpha"
		"font"				"MenuMainTitle"
		"bgcolor_override"	"36 33 32 255"
	}
	
	"WelcomeLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"			"WelcomeLabel"
		"xpos"				"15"
		"ypos"				"3"
		"zpos"				"6"
		"wide"				"1"
		"tall"				"1"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"0"
		"alpha"				"255"
		"labelText"			"Official servers"
		"textAlignment"		"west"
		"font"				"HudFontSmallishBold"
		"fgcolor"			"AdvTextDefault"
	}	
	
	"ServerList"
	{
		"ControlName"	"SectionedListPanel"
		"fieldName"		"ServerList"
		"xpos"			"2"
		"ypos"			"19"
		"zpos"			"1"
		"wide"			"1"
		"tall"			"1"
		"autoResize"	"0"
		"pinCorner"		"0"
		"font"			"Link"
		"visible"		"1"
		"enabled"		"0"
		"linespacing"	"12"
		"tabPosition"	"0"
	}	
	
	"ListSlider"
	{
		"ControlName"		"CTFAdvSlider"
		"fieldName"			"ListSlider"
		"xpos"				"252"
		"ypos"				"30"
		"zpos"				"10"
		"wide"				"1"
		"tall"				"1"
		"visible"			"1"
		"enabled"			"0"
		"minvalue" 			"0"
		"maxvalue" 			"100"
		"labelwidth" 		"1"
		"bordervisible"		"0"
		"command"			""
		"vertical"			"1"
		"value_visible"		"0"
		
		"SubButton"
		{
			"labelText" 		""
			"textAlignment"		"west"
			"font"				"FontStorePromotion"
			"border_default"	""
			"border_armed"		""	
			"border_depressed"	""	
		}
	}	
	
	"ConnectButton"
	{
		"ControlName"	"CTFAdvButton"
		"fieldName"		"ConnectButton"
		"xpos"			"200"
		"ypos"			"370"
		"zpos"			"2"
		"wide"			"1"
		"tall"			"1"
		"visible"		"1"
		"enabled"		"0"
		"command"		""
		
		"SubButton"
		{
			"labelText" 		"connect"
			"textAlignment"		"center"
			//"font"				"Link"
			"font"				"ItemFontNameLarge"
			"defaultFgColor_override"		"MainMenuTextDefault"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDepressed"	
		}
	}
}		