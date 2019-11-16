"Resource/UI/main_menu/MainMenuPanel.res"
{
	
	"CTFMainMenuPanel"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"CTFMainMenuPanel"
		"xpos"				"0"
		"ypos"				"0"
		"wide"				"f0"
		"tall"				"f0"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
	}	
	
	
	
	"FakeBGImage"
	{
		"ControlName"		"ImagePanel"
		"fieldName"			"FakeBGImage"
		"xpos"				"0"
		"ypos"				"0"	
		"zpos"				"200"		
		"wide"				"f0"
		"tall"				"f0"
		"visible"			"0"
		"enabled"			"1"
		"image"				"../console/background03_widescreen"
		"alpha"				"255"
		"scaleImage"		"1"	
	}	
	
	// ---------- Header Content ----------
	
	"HeaderBackground"
	{
		"ControlName"		"ImagePanel"
		"fieldName"			"HeaderBackground"
		"xpos"				"0"
		"ypos"				"0"
		"zpos"				"-100"
		"wide"				"f0"
		"tall"				"90"
		"visible"			"1"
		"enabled"			"1"
		"image"				"loadout_bottom_gradient"
		"scaleImage"		"0"
		"tileImage"			"1"
	}				
	"HeaderLine"
	{
		"ControlName"		"ImagePanel"
		"fieldName"			"HeaderLine"
		"xpos"				"0"
		"ypos"				"90"
		"zpos"				"70"
		"wide"				"f0"
		"tall"				"10"
		"visible"			"1"
		"enabled"			"1"
		"image"				"loadout_solid_line"
		"scaleImage"		"1"
	}	
	
	"TopLeftDataPanel"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"TopLeftDataPanel"
		"xpos"				"10"
		"ypos"				"30"
		"zpos"				"-60"
		"wide"				"p0.48"
		"tall"				"60"
		"autoResize"		"0"
		"visible"			"1"
		"enabled"			"1"
		"border"			"MainMenuBGBorderAlpha"
		"font"				"MenuMainTitle"
		"bgcolor_override"	"46 43 42 255"
	}
	
	"WelcomeLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"			"WelcomeLabel"
		"xpos"				"70"
		"ypos"				"35"
		"zpos"				"10"
		"wide"				"p0.37"
		"tall"				"15"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
		"alpha"				"255"
		"labelText"			"#WelcomeBack"
		"textAlignment"		"west"
		"font"				"HudFontSmallBold"
		"fgcolor"			"AdvTextDefault"
	}
	
	"VersionLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"			"VersionLabel"
		"xpos"				"70"
		"ypos"				"50"
		"zpos"				"10"
		"wide"				"100"
		"tall"				"20"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
		"alpha"				"255"
		"textAlignment"		"north-west"
		"font"				"HudFontSmallest"
		"fgcolor_override"	"117 107 94 255"
	}
	
	"AvatarBackground"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"AvatarBackground"
		"xpos"				"15"
		"ypos"				"10"
		"zpos"				"10"
		"wide"				"50"
		"tall"				"50"
		"visible"			"1"
		"enabled"			"1"
		"border"			"MainMenuBGBorderAlpha"
		"font"				"MenuMainTitle"
		"bgcolor_override"	"117 107 94 255"
	}
	
	"AvatarImage"
	{
		"ControlName"		"CAvatarImagePanel"
		"fieldName"			"AvatarImage"
		"xpos"				"20"
		"ypos"				"15"
		"zpos"				"20"
		"wide"				"40"
		"tall"				"40"
		"visible"			"1"
		"enabled"			"1"
		"scaleImage"		"1"
		"color_outline"		"255 255 255 255"
	}
	
	"StatsButton"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"StatsButton"
		"xpos"				"cp-0.12"
		"ypos"				"60"
		"zpos"				"10"
		"wide"				"p0.1"
		"tall"				"20"
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"
		"labelText" 		""
		"command"			"newstats"
		
		"SubButton"
		{
			"labelText" 			"#Stats"
			"bordervisible"					"0"
			"xshift" 						"0"
			"yshift" 						"0"
			"font"							"HudFontSmallestBold"
			"textAlignment"					"Center"
			"border_default"				"OldAdvButtonDefault"
			"border_armed"					"OldAdvButtonDefaultArmed"
			"border_depressed"				"OldAdvButtonDefaultArmed"
			"depressedFgColor_override"		"46 43 42 255"
		}
	
	}		
	
	//"TopRightDataPanel"
	//{
	//	"ControlName"		"EditablePanel"
	//	"fieldName"			"TopRightDataPanel"
	//	"xpos"				"c5"
	//	"ypos"				"30"
	//	"zpos"				"-60"
	//	"wide"				"p0.48"
	//	"tall"				"60"
	//	"autoResize"		"0"
	//	"visible"			"1"
	//	"enabled"			"1"
	//	"border"			"MainMenuBGBorderAlpha"
	//	"font"				"MenuMainTitle"
	//	"bgcolor_override"	"46 43 42 255"
	//}
	
	// ---------- Central Screen Content ----------
	
	"Logo"
	{
		"ControlName"		"ImagePanel"
		"fieldName"			"Logo"
		"xpos"				"c-295"
		"ypos"				"100"
		"zpos"				"0"		
		"wide"				"450"
		"tall"				"225"
		"visible"			"1"
		"enabled"			"1"
		"image"				"../vgui/main_menu/TF2_Vintage_Logo_NoCircle"
		"alpha"				"255"
		"scaleImage"		"1"	
	}
	
	"CentralMenuBackground"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"MainMenuButtonsBG"
		"xpos"				"c-295"
		"ypos"				"220"
		"zpos"				"-50"
		"wide"				"260"
		"tall"				"65"
		"visible"			"1"
		"enabled"			"1"
		"border"			"MainMenuBGBorderAlpha"
		"font"				"MenuMainTitle"
	}
	
	"ServerBrowserButton"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"ServerBrowserButton"
		"xpos"				"c-290"
		"ypos"				"225"
		"zpos"				"5"
		"wide"				"250"
		"tall"				"26"
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"		
		"command"			"gamemenucommand OpenServerBrowser"		
		
		"SubButton"
		{
			"labelText" 					"#MMenu_ChangeServer"
			"xshift" 						"6"
			"yshift" 						"0"
			"textAlignment"					"west"
			"font"							"HudFontSmallBold"
			"defaultFgColor_override"		"MainMenuTextDefault"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDepressed"	
		}
		
		"SubImage"
		{
			"image" 			"../vgui/glyph_server"
			"imagewidth"		"18"	
		}
	}
	
	"CreateServerButton"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"CreateServerButton"
		"xpos"				"c-65"
		"ypos"				"228"
		"zpos"				"10"
		"wide"				"20"
		"tall"				"20"
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"		
		"command"			"gamemenucommand OpenCreateMultiplayerGameDialog"		
		
		"SubButton"
		{
			"labelText" 					""
			"tooltip" 						"#MMenu_PlayList_CreateServer_Button"
			"bordervisible"					"0"
			"textAlignment"					"west"
			"font"							"HudFontSmallBold"
			"defaultFgColor_override"		"MainMenuTextArmed"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDefault"
			"border_default"				"MainMenuAdvButtonDepressed"
			"border_depressed"				"MainMenuAdvButton"		
		}
		
		"SubImage"
		{
			"image" 						"../vgui/glyph_create"
			"imagewidth"					"16"	
		}
	}
	
	"LoadoutButton"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"LoadoutButton"
		"xpos"				"c-290"
		"ypos"				"255"
		"zpos"				"5"
		"wide"				"250"
		"tall"				"26"
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"		
		"command"			"newloadout"
		
		"SubButton"
		{
			"labelText" 					"#GameUI_GameMenu_CharacterSetup"
			"xshift" 						"6"
			"yshift" 						"0"
			"textAlignment"					"west"
			"font"							"HudFontSmallBold"
			"defaultFgColor_override"		"MainMenuTextDefault"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDepressed"	
		}
		
		
		"SubImage"
		{
			"image" 						"../vgui/glyph_items"
			"imagewidth"					"16"	
		}
	}
	
	// ---------- Footer Content ----------
	
	"FooterBackground"
	{
		"ControlName"		"ImagePanel"
		"fieldName"			"FooterBackground"
		"xpos"				"0"
		"ypos"				"420"
		"zpos"				"-100"
		"wide"				"f0"
		"tall"				"62"
		"visible"			"1"
		"enabled"			"1"
		"image"				"loadout_bottom_gradient"
		"scaleImage"		"0"
		"tileImage"			"1"
	}				
	"FooterLine"
	{
		"ControlName"		"ImagePanel"
		"fieldName"			"FooterLine"
		"xpos"				"0"
		"ypos"				"420"
		"zpos"				"-50"
		"wide"				"f0"
		"tall"				"10"
		"visible"			"1"
		"enabled"			"1"
		"image"				"loadout_solid_line"
		"scaleImage"		"1"
	}	
	
	"DisconnectButton"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"DisconnectButton"
		"xpos"				"c-290"
		"ypos"				"437"
		"zpos"				"10"
		"wide"				"150"
		"tall"				"25"
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"	
		"Command"			"gamemenucommand Disconnect"
		"SubButton"
		{
			"labelText" 					"#GameUI_GameMenu_Disconnect"
			"bordervisible"					"1"
			"xshift" 						"4"
			"yshift" 						"0"
			"textAlignment"					"west"
			"font"							"HudFontSmallBold"
			"border_default"				"OldAdvButtonDefault"
			"border_armed"					"OldAdvButtonDefaultArmed"
			"border_depressed"				"OldAdvButtonDefaultArmed"
			"depressedFgColor_override"		"46 43 42 255"	
		}
		
		"SubImage"
		{
			"image" 						"../vgui/glyph_quit"
			"imagewidth"					"14"
			"imagecolordepressed"			"MainMenuAdvButtonDepressed"
		}			
	}
	
	"MusicToggleCheck"
	{
		"ControlName"		"CTFAdvCheckButton"
		"fieldName"			"MusicToggleCheck"
		"xpos"				"c15"
		"ypos"				"437"
		"zpos"				"10"
		"wide"				"25"
		"tall"				"25"
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"
		"labelText" 		""
		"command"			"tf2v_mainmenu_music"	
		"valuetrue"			"0"
		"valuefalse"		"1"		

		
		"SubButton"
		{
			"labelText" 					""
			"bordervisible"					"0"
			"tooltip" 						"Music Toggle"
			"textAlignment"					"center"
			"font"							"MenuSmallFont"	
			"border_default"				"OldAdvButtonDefault"
			"border_armed"					"OldAdvButtonDefaultArmed"
			"border_depressed"				"OldAdvButtonDefaultArmed"
			"depressedFgColor_override"		"46 43 42 255"	
		}
		
		"SubImage"
		{
			"image" 						"../vgui/main_menu/glyph_speaker"
			"imagecheck" 					"../vgui/main_menu/glyph_disabled"	
			"imagewidth"					"14"
		}
	}	
	
	"RandomMusicButton"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"RandomMusicButton"
		"xpos"				"c45"
		"ypos"				"437"
		"zpos"				"10"
		"wide"				"25"
		"tall"				"25"
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"
		"labelText" 		""
		"command"			"randommusic"			
		
		"SubButton"
		{
			"labelText" 					""
			"bordervisible"					"0"
			"tooltip" 						"Random Music"
			"textAlignment"					"center"
			"font"							"MenuSmallFont"	
			"border_default"				"OldAdvButtonDefault"
			"border_armed"					"OldAdvButtonDefaultArmed"
			"border_depressed"				"OldAdvButtonDefaultArmed"
			"depressedFgColor_override"		"46 43 42 255"
		}
		
		"SubImage"
		{
			"image" 						"../vgui/main_menu/glyph_random"
			"imagewidth"					"14"
		}
	}

	"MuteButton"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"MuteButton"
		"xpos"			"c75"
		"ypos"				"437"
		"zpos"				"10"
		"wide"				"25"
		"tall"				"25"
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"
		"labelText" 		""
		"command"			"gamemenucommand openplayerlistdialog"			
		
		"SubButton"
		{
			"labelText" 					""
			"bordervisible"					"0"
			"tooltip" 						"#MMenu_MutePlayers"
			"textAlignment"					"center"
			"font"							"MenuSmallFont"	
			"border_default"				"OldAdvButtonDefault"
			"border_armed"					"OldAdvButtonDefaultArmed"
			"border_depressed"				"OldAdvButtonDefaultArmed"
			"depressedFgColor_override"		"46 43 42 255"
		}
		
		"SubImage"
		{
			"image" 						"../vgui/glyph_muted"
			"imagewidth"					"14"
		}
	}
	
	"OptionsButton"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"OptionsButton"
		"xpos"				"c110"
		"ypos"				"437"
		"zpos"				"10"
		"wide"				"150"
		"tall"				"25"
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"	
		"Command"			"newoptionsdialog"
		"SubButton"
		{
			"labelText" 					"#GameUI_GameMenu_Options"
			"bordervisible"					"1"
			"xshift" 						"4"
			"yshift" 						"0"
			"textAlignment"					"west"
			"font"							"HudFontSmallBold"	
			"border_default"				"OldAdvButtonDefault"
			"border_armed"					"OldAdvButtonDefaultArmed"
			"border_depressed"				"OldAdvButtonDefaultArmed"
			"depressedFgColor_override"		"46 43 42 255"
		}
		"SubImage"
		{
			"image" 						"../vgui/glyph_options"
			"imagewidth"					"14"
			"imagecolordepressed"			"MainMenuAdvButtonDepressed"
		}	
		
				
	}	

	"OptionsOldButton"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"OptionsOldButton"
		"xpos"				"c265"
		"ypos"				"437"
		"zpos"				"10"
		"wide"				"25"
		"tall"				"25"
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"	
		"Command"			"gamemenucommand openoptionsdialog"
		"SubButton"
		{
			"labelText" 					""
			"tooltip"						"#TF_Quickplay_AdvancedOptions"
			"bordervisible"					"1"
			"xshift" 						"0"
			"yshift" 						"0"
			"textAlignment"					"west"
			"font"							"HudFontSmallBold"	
			"border_default"				"OldAdvButtonDefault"
			"border_armed"					"OldAdvButtonDefaultArmed"
			"border_depressed"				"OldAdvButtonDefaultArmed"
			"depressedFgColor_override"		"46 43 42 255"
		}
		
		"SubImage"
		{
			"image" 						"../vgui/glyph_steamworkshop"
			"imagewidth"					"14"
		}			
	}
}