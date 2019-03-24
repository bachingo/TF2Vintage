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
		"ControlName"	"ImagePanel"
		"fieldName"		"FakeBGImage"
		"xpos"			"0"
		"ypos"			"0"	
		"zpos"			"200"		
		"wide"			"f0"
		"tall"			"f0"
		"visible"		"0"
		"enabled"		"1"
		"image"			"../console/background03_widescreen"
		"alpha"			"255"
		"scaleImage"	"1"	
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
	
	"AvatarImage"
	{
		"ControlName"	"CAvatarImagePanel"
		"fieldName"		"AvatarImage"
		"xpos"			"35"
		"ypos"			"245"
		"zpos"			"6"
		"wide"			"20"
		"tall"			"20"
		"visible"		"1"
		"enabled"		"1"
		"image"			""
		"scaleImage"		"1"	
		"color_outline"		"52 48 45 255"
	}	
	"WelcomeLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"			"WelcomeLabel"
		"xpos"				"35"
		"ypos"				"220"
		"zpos"				"6"
		"wide"				"190"
		"tall"				"20"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
		"alpha"				"255"
		"labelText"			"Welcome back,"
		"textAlignment"		"west"
		"font"				"HudFontMediumSmallBold"
		"fgcolor"			"AdvTextDefault"
	}
	
	"WelcomeLabel1"
	{
		"ControlName"		"CExLabel"
		"fieldName"			"WelcomeLabel1"
		"xpos"				"39"
		"ypos"				"224"
		"zpos"				"5"
		"wide"				"190"
		"tall"				"20"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
		"alpha"				"0"
		"labelText"			"Welcome back,"
		"textAlignment"		"west"
		"font"				"HudFontMediumSmallBold_Shadow"
		"fgcolor"			"AdvTextDefault"
	}	
	
	"NicknameLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"			"NicknameLabel"
		"xpos"				"60"
		"ypos"				"245"
		"zpos"				"5"
		"wide"				"190"
		"tall"				"20"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
		"labelText"			"%nickname%"
		"textAlignment"		"west"
		"font"				"HudFontMediumSmallBold"
		"fgcolor"			"AdvTextDefault"
	}
	
	"ServerBrowserButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"ServerBrowserButton"
		"xpos"					"35"
		"ypos"					"275"
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
		"ypos"					"300"
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
		"ypos"					"375"
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
			"tooltip" 			"Advanced Options"
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
	"StatsButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"StatsButton"
		"xpos"					"35"
		"ypos"					"350"
		"zpos"					"5"
		"wide"					"250"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"0"		
		"command"				"newstats"
		
		"SubButton"
		{
			"labelText" 		"#GameUI_GameMenu_PlayerStats"
			"xshift" 			"20"
			"yshift" 			"0"
			"textAlignment"		"west"
			"font"				"HudFontSmallBold"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDefault"	
		}
		
		
		"SubImage"
		{
			"image" 			"../vgui/main_menu/glyph_stats"
			"imagewidth"		"16"	
		}
	}	
	"MusicToggleCheck"
	{
		"ControlName"			"CTFAdvCheckButton"
		"fieldName"				"MusicToggleCheck"
		"xpos"					"90"
		"ypos"					"450"
		"zpos"					"5"
		"wide"					"20"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"1"		
		"command"			"tf2c_mainmenu_music"	
		"valuetrue"			"0"
		"valuefalse"		"1"		
		
		"SubButton"
		{
			"labelText"			""
			"tooltip" 		"Music Toggle"
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
			"image" 			"../vgui/main_menu/glyph_speaker"
			"imagecheck" 		"../vgui/main_menu/glyph_disabled"	
			"imagewidth"		"16"	
		}
	}		
	"RandomMusicButton"
	{
		"ControlName"			"CTFAdvButton"
		"fieldName"				"RandomMusicButton"
		"xpos"					"65"
		"ypos"					"450"
		"zpos"					"5"
		"wide"					"20"
		"tall"					"20"
		"visible"				"1"
		"enabled"				"1"
		"bordervisible"			"1"		
		"command"				"randommusic"	
		
		"SubButton"
		{
			"labeltext"			""
			"tooltip" 		"Random Music"
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
			"image" 			"../vgui/main_menu/glyph_random"
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
		"ypos"					"325"
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
		"VersionLabel"
	{
		"ControlName"		"CExLabel"
		"fieldName"			"VersionLabel"
		"xpos"				"r525"
		"ypos"				"0"
		"zpos"				"5"
		"wide"				"520"
		"tall"				"40"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
		"labelText"			"VersionLabel"
		"textAlignment"		"east"
		"fgcolor"			"HudOffWhite"
		"font"				"MenuSmallFont"
	}
}
		