"Resource/UI/main_menu/PauseMenuPanel.res"
{
	"CTFPauseMenuPanel"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"CTFPauseMenuPanel"
		"xpos"				"0"
		"ypos"				"0"
		"wide"				"f0"
		"tall"				"f0"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
	}
	
	
	"Logo"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"Logo"
		"xpos"			"15"
		"ypos"			"45"	
		"zpos"			"3"		
		"wide"			"450"
		"tall"			"225"
		"visible"		"1"
		"enabled"		"1"
		"image"			"../vgui/main_menu/TF2_Vintage_Logo_NoCircle"
		"alpha"			"255"
		"scaleImage"	"1"	
	}	
		"ResumeButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"ResumeButton"
		"xpos"					"35"
		"ypos"					"225"
		"zpos"					"5"
		"wide"					"250"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"0"		
		"command"				"gamemenucommand ResumeGame"	
		
		"SubButton"
		{
			"labelText" 		"#GameUI_GameMenu_ResumeGame"
			"xshift" 			"20"
			"yshift" 			"0"
			"textAlignment"		"west"
			"font"				"HudFontSmallBold"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDefault"	
		}
		
		"SubImage"
		{
			"image" 			"../vgui/icon_resume"
			"imagewidth"		"16"	
		}
	}
			"MuteButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"MuteButton"
		"xpos"					"35"
		"ypos"					"250"
		"zpos"					"5"
		"wide"					"250"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"0"		
		"command"				"gamemenucommand openplayerlistdialog"
		
		"SubButton"
		{
			"labelText" 		"#GameUI_GameMenu_PlayerList"
			"xshift" 			"20"
			"yshift" 			"0"
			"textAlignment"		"west"
			"font"				"HudFontSmallBold"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDefault"	
		}
		
		"SubImage"
		{
			"image" 			"../vgui/glyph_muted"
			"imagewidth"		"16"	
		}
	}
	
	"ServerBrowserButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"ServerBrowserButton"
		"xpos"					"35"
		"ypos"					"300"
		"zpos"					"5"
		"wide"					"250"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"0"		
		"command"				"gamemenucommand OpenServerBrowser"		
		
		"SubButton"
		{
			"labelText" 		"#GameUI_GameMenu_FindServers"
			"xshift" 			"20"
			"yshift" 			"0"
			"textAlignment"		"west"
			"font"				"HudFontSmallBold"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDefault"	
		}
		
		"SubImage"
		{
			"image" 			"../vgui/glyph_server_browser"
			"imagewidth"		"16"	
		}
	}
	
		"CreateServerButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"CreateServerButton"
		"xpos"					"35"
		"ypos"					"325"
		"zpos"					"5"
		"wide"					"250"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"0"		
		"command"				"gamemenucommand OpenCreateMultiplayerGameDialog"	
		
		"SubButton"
		{
			"labelText" 		"#GameUI_GameMenu_CreateServer"
			"xshift" 			"20"
			"yshift" 			"0"
			"textAlignment"		"west"
			"font"				"HudFontSmallBold"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDefault"	
		}
		
		"SubImage"
		{
			"image" 			"../vgui/glyph_create"
			"imagewidth"		"16"	
		}
	}
	
			"OptionsButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"OptionsButton"
		"xpos"					"35"
		"ypos"					"350"
		"zpos"					"5"
		"wide"					"250"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"0"		
		"command"				"newoptionsdialog"
		
		"SubButton"
		{
			"labelText"			"#GameUI_GameMenu_Options"
			"xshift" 			"20"
			"yshift" 			"0"
			"textAlignment"		"west"
			"font"				"HudFontSmallBold"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDefault"	
		}
		
		"SubImage"
		{
			"image" 			"../vgui/glyph_options"
			"imagewidth"		"16"	
		}
	}		
	"OptionsOldButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"OptionsOldButton"	
		"xpos"					"15"
		"ypos"					"450"
		"zpos"					"5"
		"wide"					"20"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"1"		
		"command"				"gamemenucommand openoptionsdialog"
		
		"SubButton"
		{
			"labelText"			""
			"tooltip" 			"#MMenu_Tooltip_AdvOptions"
			"xshift" 			"20"
			"yshift" 			"0"
			"textAlignment"		"center"
			"font"				"MenuSmallFont"
			"border_default"	"AdvRightButtonDefault"
			"border_armed"		"AdvRightButtonArmed"
			"border_depressed"	"AdvRightButtonDepressed"	
		}
		
		"SubImage"
		{
			"image" 			"../vgui/glyph_steamworkshop"
			"imagewidth"		"16"	
		}
	}	
	"QuitButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"QuitButton"
		"xpos"					"35"
		"ypos"					"400"
		"zpos"					"5"
		"wide"					"250"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"0"		
		"command"				"newquit"
		
		"SubButton"
		{
			"labelText" 		"#GameUI_GameMenu_Quit"
			"xshift" 			"20"
			"yshift" 			"0"
			"textAlignment"		"west"
			"font"				"HudFontSmallBold"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDefault"	
		}
		
		
		"SubImage"
		{
			"image" 			"../vgui/glyph_quit"
			"imagewidth"		"16"	
		}
	}
	"DisconnectButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"DisconnectButton"
		"xpos"					"35"
		"ypos"					"375"
		"zpos"					"5"
		"wide"					"250"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"0"		
		"command"				"gamemenucommand Disconnect"
		
		"SubButton"
		{
			"labelText" 		"#GameUI_GameMenu_Disconnect"
			"xshift" 			"20"
			"yshift" 			"0"
			"textAlignment"		"west"
			"font"				"HudFontSmallBold"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDefault"	
		}
		
		
		"SubImage"
		{
			"image" 			"../vgui/glyph_server"
			"imagewidth"		"16"	
		}
	}		
	"TheaterButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"TheaterButton"
		"xpos"					"40"
		"ypos"					"450"
		"zpos"					"5"
		"wide"					"20"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"1"		
		"command"				"demoui2"
		
		"SubButton"
		{
			"labelText"			""
			"tooltip" 		"#MMenu_Tooltip_Replay"
			"xshift" 			"20"
			"yshift" 			"0"
			"textAlignment"		"center"
			"font"				"MenuSmallFont"	
			"border_default"	"AdvRightButtonDefault"
			"border_armed"		"AdvRightButtonArmed"
			"border_depressed"	"AdvRightButtonDepressed"
		}
		
		
		"SubImage"
		{
			"image" 			"../vgui/glyph_tv"
			"imagewidth"		"16"	
		}
	}	
	"LoadoutButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"LoadoutButton"
		"xpos"					"35"
		"ypos"					"275"
		"zpos"					"5"
		"wide"					"250"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"0"		
		"command"				"newloadout"
		
		"SubButton"
		{
			"labelText" 		"#GameUI_GameMenu_CharacterSetup"
			"xshift" 			"20"
			"yshift" 			"0"
			"textAlignment"		"west"
			"font"				"HudFontSmallBold"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDefault"	
		}
		
		
		"SubImage"
		{
			"image" 			"../vgui/glyph_items"
			"imagewidth"		"18"	
		}
	}	

}
		