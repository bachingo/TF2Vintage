"Resource/UI/main_menu/LoadoutPanel.res"
{		
	"CTFLoadoutPanel"
	{
		"ControlName"		"EditablePanel"
		"fieldName"			"CTFLoadoutPanel"
		"xpos"				"0"
		"ypos"				"0"
		"wide"				"f0"
		"tall"				"f0"
		"autoResize"		"0"
		"pinCorner"			"0"
		"visible"			"1"
		"enabled"			"1"
		"border"			""
	}
	
	"BackgroundImage"
	{
		"ControlName"	"EditablePanel"
		"fieldName"		"BackgroundImage"
		"xpos"			"0"
		"ypos"			"0"	
		"zpos"			"-200"		
		"wide"			"f0"
		"tall"			"f0"
		"visible"		"1"
		"enabled"		"1"
		"scaleImage"	"1"	
		"bgcolor_override"	"42 39 37 255"
	}	
	
	"BackgroundHeader"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"BackgroundHeader"
		"xpos"			"0"
		"ypos"			"0"
		"zpos"			"-100"
		"wide"			"f0"
		"tall"			"54"
		"visible"		"1"
		"enabled"		"1"
		"image"			"loadout_header"
		"scaleImage"	"0"
		"tileImage"		"1"
	}	

	"HeaderLine"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"HeaderLine"
		"xpos"			"0"
		"ypos"			"54"
		"zpos"			"-99"
		"wide"			"f0"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"
		"image"			"loadout_solid_line"
		"scaleImage"	"1"
	}
	
	"BackgroundFooter"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"BackgroundFooter"
		"xpos"			"0"
		"ypos"			"420"
		"zpos"			"-100"
		"wide"			"f0"
		"tall"			"60"
		"visible"		"1"
		"enabled"		"1"
		"image"			"loadout_bottom_gradient"
		"tileImage"		"1"
	}
	
	"CaratLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"CaratLabel"
		"font"			"HudFontSmallestBold"
		"labelText"		">>"
		"textAlignment"	"west"
		"xpos"			"85"
		"ypos"			"20"
		"zpos"			"10"
		"wide"			"20"
		"tall"			"15"
		"autoResize"	"1"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"fgcolor_override" "200 80 60 255"
	}
	
	"HackTitleLabel"
	{
		"ControlName"	"CExLabel"
		"fieldName"		"HackTitleLabel"
		"font"			"HudFontMediumSmallBold"
		"labelText"		"#CharInfoAndSetup"
		"textAlignment"	"west"
		"xpos"			"105"
		"ypos"			"20"
		"zpos"			"10"
		"wide"			"350"
		"tall"			"15"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"			"1"
		"fgcolor_override"	"117 107 94 255"
	}
	
	"FooterLine"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"FooterLine"
		"xpos"			"0"
		"ypos"			"420"
		"zpos"			"-99"
		"wide"			"f0"
		"tall"			"10"
		"visible"		"1"
		"enabled"		"1"
		"image"			"loadout_solid_line"
		"scaleImage"	"1"
	}	
	
	"TopDotted"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"TopDotted"
		"xpos"			"cs-0.5"
		"ypos"			"70"
		"zpos"			"-100"
		"wide"			"f80"
		"tall"			"8"
		"visible"		"1"
		"enabled"		"1"
		"image"			"loadout_dotted_line"
		"scaleImage"	"0"
		"tileImage"		"1"
	}	
	
	"BottomDotted"
	{
		"ControlName"	"ImagePanel"
		"fieldName"		"BottomDotted"
		"xpos"			"cs-0.5"
		"ypos"			"r126"
		"zpos"			"-100"
		"wide"			"f80"
		"tall"			"8"
		"visible"		"1"
		"enabled"		"1"
		"image"			"loadout_dotted_line"
		"scaleImage"	"0"
		"tileImage"		"1"
	}	
	
	"EquipLabel"
	{
		"ControlName"		"Label"
		"fieldName"		"EquipLabel"
		"font"			"HudFontMediumBigBold"
		"labelText"		"#EquipYourClass"
		"textAlignment"	"north-west"
		"xpos"			"0"
		"ypos"			"35"
		"zpos"			"1"
		"wide"			"f0"
		"tall"			"30"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"0"
		"enabled"		"1"
	}
	
	"weaponbutton0"
	{
		"ControlName"	"CTFAdvItemButton"
		"fieldName"		"weaponbutton0"		
		"xpos"			"c-280"
		"ypos"			"94"		
		"zpos"			"-1"		
		"wide"			"140"
		"tall"			"74"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		//"border"		"MainMenuHighlightBorder"
		"SubButton"
		{
			"font"				"HudFontSmallestBold"
			"border_default"	"AdvRoundedButtonDefault"
			"border_armed"		"AdvRoundedButtonArmed"
			"border_depressed"	"AdvRoundedButtonDepressed"	
		}
	}
	
	"weaponbutton1"
	{
		"ControlName"	"CTFAdvItemButton"
		"fieldName"		"weaponbutton1"		
		"xpos"			"c-280"
		"ypos"			"178"
		"zpos"			"-1"		
		"wide"			"140"
		"tall"			"74"
		"autoResize"	"5"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		//"border"		"MainMenuHighlightBorder"
		"SubButton"
		{
			"font"				"HudFontSmallestBold"
			"border_default"	"AdvRoundedButtonDefault"
			"border_armed"		"AdvRoundedButtonArmed"
			"border_depressed"	"AdvRoundedButtonDepressed"	
		}
	}
	
	"weaponbutton2"
	{
		"ControlName"	"CTFAdvItemButton"
		"fieldName"		"weaponbutton2"	
		"xpos"			"c-280"
		"ypos"			"262"
		"zpos"			"-1"		
		"wide"			"140"
		"tall"			"74"
		"autoResize"	"5"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"SubButton"
		{
			"font"				"HudFontSmallestBold"
			"border_default"	"AdvRoundedButtonDefault"
			"border_armed"		"AdvRoundedButtonArmed"
			"border_depressed"	"AdvRoundedButtonDepressed"	
		}
	}
	
	"weaponbutton3"
	{
		"ControlName"	"CTFAdvItemButton"
		"fieldName"		"weaponbutton3"		
		"xpos"			"c140"
		"ypos"			"94"		
		"zpos"			"-1"		
		"wide"			"105"
		"tall"			"56"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"SubButton"
		{
			"font"				"HudFontSmallestBold"
			"border_default"	"AdvRoundedButtonDefault"
			"border_armed"		"AdvRoundedButtonArmed"
			"border_depressed"	"AdvRoundedButtonDepressed"	
		}
	}
	
	"weaponbutton4"
	{
		"ControlName"	"CTFAdvItemButton"
		"fieldName"		"weaponbutton4"		
		"xpos"			"c140"
		"ypos"			"160"
		"zpos"			"-1"		
		"wide"			"105"
		"tall"			"56"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"SubButton"
		{
			"font"				"HudFontSmallestBold"
			"border_default"	"AdvRoundedButtonDefault"
			"border_armed"		"AdvRoundedButtonArmed"
			"border_depressed"	"AdvRoundedButtonDepressed"	
		}
	}
	
	"weaponbutton5"
	{
		"ControlName"	"CTFAdvItemButton"
		"fieldName"		"weaponbutton5"		
		"xpos"			"c140"
		"ypos"			"226"
		"zpos"			"-1"		
		"wide"			"105"
		"tall"			"56"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"SubButton"
		{
			"font"				"HudFontSmallestBold"
			"border_default"	"AdvRoundedButtonDefault"
			"border_armed"		"AdvRoundedButtonArmed"
			"border_depressed"	"AdvRoundedButtonDepressed"	
		}
	}
	
	"weaponbutton6"
	{
		"ControlName"	"CTFAdvItemButton"
		"fieldName"		"weaponbutton5"		
		"xpos"			"c140"
		"ypos"			"292"
		"zpos"			"-1"		
		"wide"			"105"
		"tall"			"56"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"SubButton"
		{
			"font"				"HudFontSmallestBold"
			"border_default"	"AdvRoundedButtonDefault"
			"border_armed"		"AdvRoundedButtonArmed"
			"border_depressed"	"AdvRoundedButtonDepressed"	
		}
	}
	
	"classmodelpanel"
	{
		"ControlName"	"CTFAdvModelPanel"
		"bgcolor_override"	"255 0 0 96"
		"xpos"			"cs-0.5"
		"ypos"			"90"
		"zpos"			"-2"		
		"wide"			"370"
		"tall"			"280"
		"autoResize"	"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		
		"fov"			"43"
		"allow_rot"		"1"
		"allow_manip"	"0"
		"allow_pitch"	"0"
		
		"model"
		{
			"force_pos"	"1"
			"skin"	"0"

			"angles_x" "0"
			"angles_y" "200"
			"angles_z" "0"
			"origin_x" "190"
			"origin_y" "0"
			"origin_z" "-36"
			"frame_origin_x"	"0"
			"frame_origin_y"	"0"
			"frame_origin_z"	"0"
			"spotlight" "1"
		
			"modelname"		"models/player/heavy.mdl"
			
			"attached_model"
			{
				"modelname" "models/weapons/w_models/w_flamethrower.mdl"
				"skin"	"0"
			}
			
			"animation"
			{
				"name"		"PRIMARY"
				"activity"	"ACT_MP_STAND_PRIMARY"
				"default"	"1"
			}
			"animation"
			{
				"name"		"SECONDARY"
				"activity"	"ACT_MP_STAND_SECONDARY"
			}
			"animation"
			{
				"name"		"MELEE"
				"activity"	"ACT_MP_STAND_MELEE"
			}
			"animation"
			{
				"name"		"GRENADE"
				"activity"	""
			}	
			"animation"
			{
				"name"		"BUILDING"
				"activity"	"ACT_MP_STAND_BUILDING"
			}	
			"animation"
			{
				"name"		"PDA"
				"activity"	"ACT_MP_STAND_PDA"
			}	
			"animation"
			{
				"name"		"ITEM1"
				"activity"	"ACT_MP_STAND_ITEM1"
			}
			"animation"
			{
				"name"		"ITEM2"
				"activity"	"ACT_MP_STAND_ITEM2"
			}
			"animation"
			{
				"name"		"MELEE_ALLCLASS"
				"activity"	"ACT_MP_STAND_MELEE_ALLCLASS"
			}
			"animation"
			{
				"name"		"SECONDARY2"
				"activity"	"ACT_MP_STAND_SECONDARY2"
			}
			"animation"
			{
				"name"		"PRIMARY2"
				"activity"	"ACT_MP_STAND_PRIMARY2"
			}
		}
	}
	
	"BackButton"
	{
		"ControlName"	"CTFAdvButton"
		"fieldName"		"BackButton"
		"xpos"			"c160"
		"ypos"			"437"
		"zpos"			"-1"
		"wide"			"100"
		"tall"			"25"
		"visible"		"1"
		"enabled"		"1"
		"command"		"back"		
		
		"SubButton"
		{
			"labelText" 		"#GameUI_Close"
			"textAlignment"		"center"
			"font"				"HudFontSmallBold"
			"border_default"	"OldAdvButtonDefault"
			"border_armed"		"OldAdvButtonDefaultArmed"
			"border_depressed"	"OldAdvButtonDefaultArmed"	
		}
	}	

	"LogoCircle"
	{
		"ControlName"	"CTFRotatingImagePanel"
		"fieldName"		"LogoCircle"
		"xpos"			"87"
		"ypos"			"44"
		"zpos"			"30"
		"wide"			"10"
		"tall"			"10"
		"image"			"vgui/class_icons/scout"
		"visible"		"0"
		"enabled"		"1"
	}
	
	"classselection"
	{
		"ControlName"		"CAdvTabs"
		"fieldName"			"classselection"
		"xpos"				"c-220"
		"ypos"				"r112"
		"zpos"				"40"
		"wide"				"440"
		"tall"				"44"
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"		
		"offset"			"0"
		
		"scout_blue"
		{
			"ControlName"		"CTFAdvButton"
			"fieldName"			"scout_blue"
			"scaleimage"		"0"	
			"visible"			"1"
			"enabled"			"1"
			"bordervisible"		"0"		
			"command"			"select_scout"
			
			"SubButton"
			{
				"labelText" 		""
				"textAlignment"		"south"
				"font"				"TallTextSmall"
				"selectedFgColor_override"		"HudProgressBarActive"
			}
			
			"SubImage"
			{
				"image" 			"class_icons/scout"	
				"imagewidth"		"44"
			}
		}
	
		"soldier_blue"
		{
			"ControlName"		"CTFAdvButton"
			"fieldName"			"soldier_blue"
			"scaleimage"		"0"	
			"visible"			"1"
			"enabled"			"1"
			"bordervisible"		"0"		
			"command"			"select_soldier"
			
			"SubButton"
			{
				"labelText" 		""
				"textAlignment"		"south"
				"font"				"TallTextSmall"
				"selectedFgColor_override"		"HudProgressBarActive"
			}
			
			"SubImage"
			{
				"image" 			"class_icons/soldier"	
				"imagewidth"		"44"
			}
		}
	
		"pyro_blue"
		{
			"ControlName"		"CTFAdvButton"
			"fieldName"			"pyro_blue"
			"scaleimage"		"0"	
			"visible"			"1"
			"enabled"			"1"
			"bordervisible"		"0"	
			"command"			"select_pyro"
			
			"SubButton"
			{
				"labelText" 		""
				"textAlignment"		"south"
				"font"				"TallTextSmall"
				"selectedFgColor_override"		"HudProgressBarActive"
			}
			
			"SubImage"
			{
				"image" 			"class_icons/pyro"	
				"imagewidth"		"44"
			}
		}
	
		"demoman_blue"
		{
			"ControlName"		"CTFAdvButton"
			"fieldName"			"demoman_blue"
			"scaleimage"		"0"	
			"visible"			"1"
			"enabled"			"1"
			"bordervisible"		"0"	
			"command"			"select_demoman"
			
			"SubButton"
			{
				"labelText" 		""
				"textAlignment"		"south"
				"font"				"TallTextSmall"
				"selectedFgColor_override"		"HudProgressBarActive"
			}
			
			"SubImage"
			{
				"image" 			"class_icons/demo"	
				"imagewidth"		"44"
			}
		}
	
		"heavyweapons_blue"
		{
			"ControlName"		"CTFAdvButton"
			"fieldName"			"heavyweapons_blue"
			"scaleimage"		"0"	
			"visible"			"1"
			"enabled"			"1"
			"bordervisible"		"0"	
			"command"			"select_heavyweapons"
			
			"SubButton"
			{
				"labelText" 		""
				"textAlignment"		"south"
				"font"				"TallTextSmall"
				"selectedFgColor_override"		"HudProgressBarActive"
			}
			
			"SubImage"
			{
				"image" 			"class_icons/heavy"	
				"imagewidth"		"44"
			}
		}
	
		"engineer_blue"
		{
			"ControlName"		"CTFAdvButton"
			"fieldName"			"engineer_blue"
			"scaleimage"		"0"	
			"visible"			"1"
			"enabled"			"1"
			"bordervisible"		"0"	
			"command"			"select_engineer"
			
			"SubButton"
			{
				"labelText" 		""
				"textAlignment"		"south"
				"font"				"TallTextSmall"
				"selectedFgColor_override"		"HudProgressBarActive"
			}
			
			"SubImage"
			{
				"image" 			"class_icons/engineer"	
				"imagewidth"		"44"
			}
		}
	
		"medic_blue"
		{
			"ControlName"		"CTFAdvButton"
			"fieldName"			"medic_blue"
			"scaleimage"		"0"	
			"visible"			"1"
			"enabled"			"1"
			"bordervisible"		"0"	
			"command"			"select_medic"
			
			"SubButton"
			{
				"labelText" 		""
				"textAlignment"		"south"
				"font"				"TallTextSmall"
				"selectedFgColor_override"		"HudProgressBarActive"
			}
			
			"SubImage"
			{
				"image" 			"class_icons/medic"	
				"imagewidth"		"44"
			}
		}
	
		"sniper_blue"
		{
			"ControlName"		"CTFAdvButton"
			"fieldName"			"sniper_blue"
			"scaleimage"		"0"	
			"visible"			"1"
			"enabled"			"1"
			"bordervisible"		"0"	
			"command"			"select_sniper"
			
			"SubButton"
			{
				"labelText" 		""
				"textAlignment"		"south"
				"font"				"TallTextSmall"
				"selectedFgColor_override"		"HudProgressBarActive"
			}
			
			"SubImage"
			{
				"image" 			"class_icons/sniper"	
				"imagewidth"		"44"
			}
		}
	
		"spy_blue"
		{
			"ControlName"		"CTFAdvButton"
			"fieldName"			"spy_blue"
			"scaleimage"		"0"	
			"visible"			"1"
			"enabled"			"1"
			"bordervisible"		"0"	
			"command"			"select_spy"
			
			"SubButton"
			{
				"labelText" 		""
				"textAlignment"		"south"
				"font"				"TallTextSmall"
				"selectedFgColor_override"		"HudProgressBarActive"
			}
			
			"SubImage"
			{
				"image" 			"class_icons/spy"	
				"imagewidth"		"44"
			}
		}
	}
	
	"preset_a"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"preset_a"
		"xpos"				"c-60"
		"ypos"				"35"
		"zpos"				"100"
		"wide"				"25"
		"tall"				"25"
		"scaleimage"		"0"	
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"		
		"command"			"loadpreset_0"
		"SubButton"
		{
			"labelText"			"#TF_ItemPresetName0"
			"textAlignment"		"center"
			"font"				"TallTextSmall"
			"defaultFgColor_override"		"MainMenuTextDefault"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDepressed"	
			"border_default"	"OldAdvButtonDefault"
			"border_armed"		"OldAdvButtonDefaultArmed"
			"border_depressed"	"OldAdvButtonDefaultArmed"	
		}
	}
	
	"preset_b"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"preset_b"
		"xpos"				"c-30"
		"ypos"				"35"
		"zpos"				"100"
		"wide"				"25"
		"tall"				"25"
		"scaleimage"		"0"	
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"		
		"command"			"loadpreset_1"
		"SubButton"
		{
			"labelText"			"#TF_ItemPresetName1"
			"textAlignment"		"center"
			"font"				"TallTextSmall"
			"defaultFgColor_override"		"MainMenuTextDefault"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDepressed"	
			"border_default"	"OldAdvButtonDefault"
			"border_armed"		"OldAdvButtonDefaultArmed"
			"border_depressed"	"OldAdvButtonDefaultArmed"	
		}

	}
	
	"preset_c"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"preset_c"
		"xpos"				"c30"
		"ypos"				"35"
		"zpos"				"100"
		"wide"				"25"
		"tall"				"25"
		"scaleimage"		"0"	
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"		
		"command"			"loadpreset_2"
		"SubButton"
		{
			"labelText"			"#TF_ItemPresetName2"
			"textAlignment"		"center"
			"font"				"TallTextSmall"
			"defaultFgColor_override"		"MainMenuTextDefault"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDepressed"	
			"border_default"	"OldAdvButtonDefault"
			"border_armed"		"OldAdvButtonDefaultArmed"
			"border_depressed"	"OldAdvButtonDefaultArmed"	
		}

	}
	
	"preset_d"
	{
		"ControlName"		"CTFAdvButton"
		"fieldName"			"preset_d"
		"xpos"				"c60"
		"ypos"				"35"
		"zpos"				"100"
		"wide"				"25"
		"tall"				"25"
		"scaleimage"		"0"	
		"visible"			"1"
		"enabled"			"1"
		"bordervisible"		"1"		
		"command"			"loadpreset_3"
		"SubButton"
		{
			"labelText" 		"#TF_ItemPresetName3"
			"textAlignment"		"center"
			"font"				"TallTextSmall"
			"defaultFgColor_override"		"MainMenuTextDefault"
			"armedFgColor_override"			"MainMenuTextArmed"
			"depressedFgColor_override"		"MainMenuTextDepressed"
			"border_default"	"OldAdvButtonDefault"
			"border_armed"		"OldAdvButtonDefaultArmed"
			"border_depressed"	"OldAdvButtonDefaultArmed"		
		}
	}
	
}
