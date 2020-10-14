#include "cbase.h"
#include "econ_item_system.h"
#include "script_parser.h"
#include "activitylist.h"

#if defined(CLIENT_DLL)
#define UTIL_VarArgs  VarArgs
#endif

const char *g_TeamVisualSections[TF_TEAM_COUNT] =
{
	"visuals",			// TEAM_UNASSIGNED
	"",					// TEAM_SPECTATOR
	"visuals_red",		// TEAM_RED
	"visuals_blu",		// TEAM_BLUE
	"visuals_grn",		// TEAM_GREEN
	"visuals_ylw",		// TEAM_YELLOW
	//"visuals_mvm_boss"	// ???
};

const char *g_WearableAnimTypeStrings[NUM_WEARABLEANIM_TYPES] =
{
	"on_spawn",
	"start_building",
	"stop_building",
	"start_taunting",
	"stop_taunting",
};

const char *g_AttributeDescriptionFormats[] =
{
	"value_is_percentage",
	"value_is_inverted_percentage",
	"value_is_additive",
	"value_is_additive_percentage",
	"value_is_or",
	"value_is_date",
	"value_is_account_id",
	"value_is_particle_index",
	"value_is_killstreakeffect_index",
	"value_is_killstreak_idleeffect_index",
	"value_is_item_def",
	"value_is_from_lookup_table"
};

const char *g_EffectTypes[] =
{
	"unusual",
	"strange",
	"neutral",
	"positive",
	"negative"
};

const char *g_szQualityStrings[] =
{
	"normal",
	"rarity1",
	"rarity2",
	"vintage",
	"rarity3",
	"rarity4",
	"unique",
	"community",
	"developer",
	"selfmade",
	"customized",
	"strange",
	"completed",
	"haunted",
	"collectors",
	"paintkitWeapon",
};

const char *g_szQualityColorStrings[] =
{
	"QualityColorNormal",
	"QualityColorrarity1",
	"QualityColorrarity2",
	"QualityColorVintage",
	"QualityColorrarity3",
	"QualityColorrarity4",
	"QualityColorUnique",
	"QualityColorCommunity",
	"QualityColorDeveloper",
	"QualityColorSelfMade",
	"QualityColorSelfMadeCustomized",
	"QualityColorStrange",
	"QualityColorCompleted",
	"QualityColorHaunted",
	"QualityColorCollectors",
	"QualityColorPaintkitWeapon",
};

const char *g_szQualityLocalizationStrings[] =
{
	"#Normal",
	"#rarity1",
	"#rarity2",
	"#vintage",
	"#rarity3",
	"#rarity4",
	"#unique",
	"#community",
	"#developer",
	"#selfmade",
	"#customized",
	"#strange",
	"#completed",
	"#haunted",
	"#collectors",
	"#paintkitWeapon",
};

const char *g_szDropTypeStrings[] =
{
	"",
	"none",
	"drop",
	"break",
};

int GetTeamVisualsFromString( const char *pszString )
{
	for ( int i = 0; i < ARRAYSIZE( g_TeamVisualSections ); i++ )
	{
		// There's a NULL hidden in g_TeamVisualSections
		if ( g_TeamVisualSections[i] && !V_stricmp( pszString, g_TeamVisualSections[i] ) )
			return i;
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static CEconItemSchema g_EconItemSchema;
CEconItemSchema *GetItemSchema()
{
	return &g_EconItemSchema;
}

class CEconSchemaParser : public CScriptParser
{
public:
	DECLARE_CLASS_GAMEROOT( CEconSchemaParser, CScriptParser );

#define GET_STRING(copyto, from, name)													\
		if (from->GetString(#name, NULL))												\
			copyto->name = from->GetString(#name)

#define GET_STRING_DEFAULT(copyto, from, name, defaultstring) \
		copyto->name = from->GetString(#name, #defaultstring)

#define GET_BOOL(copyto, from, name) \
		copyto->name = from->GetBool(#name, copyto->name)

#define GET_FLOAT(copyto, from, name) \
		copyto->name = from->GetFloat(#name, copyto->name)

#define GET_INT(copyto, from, name) \
		copyto->name = from->GetInt(#name, copyto->name)

#define GET_STRING_CONVERT(copyto, from, name) \
		if (from->GetString(#name, NULL))

#define FIND_ELEMENT(map, key, val)						\
		unsigned int index = map.Find(key);				\
		if (index != map.InvalidIndex())				\
			val = map.Element(index)				

#define FIND_ELEMENT_STRING(map, key, val)						\
		unsigned int index = map.Find(key);						\
		if (index != map.InvalidIndex())						\
			Q_snprintf(val, sizeof(val), map.Element(index))

#define IF_ELEMENT_FOUND(map, key)						\
		unsigned int index = map.Find(key);				\
		if (index != map.InvalidIndex())			

#define GET_VALUES_FAST_BOOL(dict, keys)\
		for (KeyValues *pKeyData = keys->GetFirstSubKey(); pKeyData != NULL; pKeyData = pKeyData->GetNextKey())\
		{													\
			IF_ELEMENT_FOUND(dict, pKeyData->GetName())		\
			{												\
				dict.Element(index) = pKeyData->GetBool();	\
			}												\
			else											\
			{												\
				dict.Insert(pKeyData->GetName(), pKeyData->GetBool());\
			}												\
		}


#define GET_VALUES_FAST_STRING(dict, keys)\
		for (KeyValues *pKeyData = keys->GetFirstSubKey(); pKeyData != NULL; pKeyData = pKeyData->GetNextKey())	\
		{													\
			IF_ELEMENT_FOUND(dict, pKeyData->GetName())		\
			{												\
				Q_snprintf((char*)dict.Element(index), sizeof(dict.Element(index)), pKeyData->GetString());		\
			}												\
			else											\
			{												\
				dict.Insert(pKeyData->GetName(), strdup(pKeyData->GetString()));\
			}												\
		}	

	void Parse( KeyValues *pKeyValuesData, bool bWildcard = false, const char *szFileWithoutEXT = NULL ) OVERRIDE
	{
		GetItemSchema()->m_pSchema = pKeyValuesData->MakeCopy();

		KeyValues *pPrefabs = pKeyValuesData->FindKey( "prefabs" );
		if ( pPrefabs )
		{
			ParsePrefabs( pPrefabs );
		}

		KeyValues *pGameInfo = pKeyValuesData->FindKey( "game_info" );
		if ( pGameInfo )
		{
			ParseGameInfo( pGameInfo );
		}

		KeyValues *pQualities = pKeyValuesData->FindKey( "qualities" );
		if ( pQualities )
		{
			ParseQualities( pQualities );
		}

		KeyValues *pColors = pKeyValuesData->FindKey( "colors" );
		if ( pColors )
		{
			ParseColors( pColors );
		}

		KeyValues *pAttributes = pKeyValuesData->FindKey( "attributes" );
		if ( pAttributes )
		{
			ParseAttributes( pAttributes );
		}

		KeyValues *pItems = pKeyValuesData->FindKey( "items" );
		if ( pItems )
		{
			ParseItems( pItems );
		}
		
		// TF2V exclusive lists start here.
		// None of these are necessary but they help in organizing the item schema.
		
		// Stockweapons is just for cataloging stock content.
		KeyValues *pStockItems = pKeyValuesData->FindKey( "stockitems" );
		if ( pStockItems )
		{
			ParseItems( pStockItems );
		}
		
		// Base Unlocks catalogs the standard unlock weapons.
		KeyValues *pUnlockItems = pKeyValuesData->FindKey( "unlockitems" );
		if ( pUnlockItems )
		{
			ParseItems( pUnlockItems );
		}
		
		// Stock Cosmetics are for the typical cosmetics.
		KeyValues *pCosmeticItems = pKeyValuesData->FindKey( "cosmeticitems" );
		if ( pCosmeticItems )
		{
			ParseItems( pCosmeticItems );
		}
		
		// Reskins is for reskin weapons.
		KeyValues *pReskinItems = pKeyValuesData->FindKey( "reskinitems" );
		if ( pReskinItems )
		{
			ParseItems( pReskinItems );
		}
		
		// Special is for special items, like medals and zombies.
		KeyValues *pSpecialItems = pKeyValuesData->FindKey( "specialitems" );
		if ( pSpecialItems )
		{
			ParseItems( pSpecialItems );
		}
		
	};

	void ParseGameInfo( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
		{
			GetItemSchema()->m_GameInfo.Insert( pSubData->GetName(), pSubData->GetFloat() );
		}
	};

	void ParseQualities( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
		{
			EconQuality Quality;
			GET_INT( ( &Quality ), pSubData, value );
			GetItemSchema()->m_Qualities.Insert( pSubData->GetName(), Quality );
		}

	};

	void ParseColors( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
		{
			EconColor ColorDesc;
			GET_STRING( ( &ColorDesc ), pSubData, color_name );
			GetItemSchema()->m_Colors.Insert( pSubData->GetName(), ColorDesc );
		}
	};

	void ParsePrefabs( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstTrueSubKey(); pSubData != NULL; pSubData = pSubData->GetNextTrueSubKey() )
		{
			if ( GetItemSchema()->m_PrefabsValues.IsValidIndex( GetItemSchema()->m_PrefabsValues.Find( pSubData->GetName() ) ) )
			{
				Error( "Duplicate prefab name (%s)\n", pSubData->GetName() );
				continue;
			}

			KeyValues *Values = pSubData->MakeCopy();
			GetItemSchema()->m_PrefabsValues.Insert( pSubData->GetName(), Values );
		}
	};

	void ParseItems( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstTrueSubKey(); pSubData != NULL; pSubData = pSubData->GetNextTrueSubKey() )
		{
			// Skip over default item, not sure why it's there.
			if ( V_stricmp( pSubData->GetName(), "default" ) == 0 )
				continue;

			CEconItemDefinition *pItem = new CEconItemDefinition;
			int index = V_atoi( pSubData->GetName() );
			pItem->index = index;

			KeyValues *pDefinition = new KeyValues( pSubData->GetName() );
			MergeDefinitionPrefabs( pDefinition, pSubData );
			pItem->LoadFromKV( pDefinition );

			GetItemSchema()->m_Items.Insert( index, pItem );
		}
	};

	void ParseAttributes( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstTrueSubKey(); pSubData != NULL; pSubData = pSubData->GetNextTrueSubKey() )
		{
			CEconAttributeDefinition *pAttribute = new CEconAttributeDefinition;
			pAttribute->index = V_atoi( pSubData->GetName() );

			pAttribute->LoadFromKV( pSubData->MakeCopy() );

			GetItemSchema()->m_Attributes.Insert( pAttribute->index, pAttribute );
		}
	};

	static bool ParseVisuals( KeyValues *pData, CEconItemDefinition *pItem, int iIndex )
	{
		PerTeamVisuals_t *pVisuals = new PerTeamVisuals_t;

		for ( KeyValues *pVisualData = pData->GetFirstSubKey(); pVisualData != NULL; pVisualData = pVisualData->GetNextKey() )
		{
			if ( !V_stricmp( pVisualData->GetName(), "use_visualsblock_as_base" ) )
			{
				const char *pszBlockTeamName = pVisualData->GetString();
				int iTeam = GetTeamVisualsFromString( pszBlockTeamName );
				if ( iTeam != -1 )
				{
					*pVisuals = *pItem->GetVisuals( iTeam );
				}
				else
				{
					Error( "Unknown visuals block: %s", pszBlockTeamName );
				}
			}
			else if ( !V_stricmp( pVisualData->GetName(), "player_bodygroups" ) )
			{
				GET_VALUES_FAST_BOOL( pVisuals->player_bodygroups, pVisualData );
			}
			else if ( !V_stricmp( pVisualData->GetName(), "attached_models" ) )
			{
				for (KeyValues *pAttachment = pVisualData->GetFirstSubKey(); pAttachment != NULL; pAttachment = pAttachment->GetNextKey())
				{
					AttachedModel_t attached_model;
					attached_model.model_display_flags = pAttachment->GetInt( "model_display_flags", AM_VIEWMODEL|AM_WORLDMODEL );
					V_strncpy( attached_model.model, pAttachment->GetString( "model" ), sizeof( attached_model.model ) );

					pVisuals->attached_models.AddToTail( attached_model );
				}
			}
			else if ( !V_stricmp( pVisualData->GetName(), "custom_particlesystem" ) )
			{
				pVisuals->custom_particlesystem = pVisualData->GetString( "system" );
			}
			else if ( !V_stricmp( pVisualData->GetName(), "muzzle_flash" ) )
			{
				// Fetching this similar to weapon script file parsing.
				pVisuals->muzzle_flash = pVisualData->GetString();
			}
			else if ( !V_stricmp( pVisualData->GetName(), "tracer_effect" ) )
			{
				// Fetching this similar to weapon script file parsing.
				pVisuals->tracer_effect = pVisualData->GetString();
			}
			else if ( !V_stricmp( pVisualData->GetName(), "animation_replacement" ) )
			{
				for ( KeyValues *pKeyData = pVisualData->GetFirstSubKey(); pKeyData != NULL; pKeyData = pKeyData->GetNextKey() )
				{
					int key = ActivityList_IndexForName( pKeyData->GetName() );
					int value = ActivityList_IndexForName( pKeyData->GetString() );

					if ( key != kActivityLookup_Missing && value != kActivityLookup_Missing )
					{
						pVisuals->animation_replacement.Insert( key, value );
					}
				}
			}
			else if ( !V_stricmp( pVisualData->GetName(), "playback_activity" ) )
			{
				for ( KeyValues *pKeyData = pVisualData->GetFirstSubKey(); pKeyData != NULL; pKeyData = pKeyData->GetNextKey() )
				{
					int iPlaybackType = UTIL_StringFieldToInt( pKeyData->GetName(), g_WearableAnimTypeStrings, NUM_WEARABLEANIM_TYPES );
					if ( iPlaybackType != -1 )
					{
						activity_on_wearable_t activity;
						activity.playback = (wearableanimplayback_t)iPlaybackType;
						activity.activity = kActivityLookup_Unknown;
						activity.activity_name = pKeyData->GetString();

						pVisuals->playback_activity.AddToTail( activity );
					}
				}
			}
			else if ( !V_strnicmp( pVisualData->GetName(), "custom_sound", 12 ) )
			{
				int iSound = 0;
				if ( pVisualData->GetName()[12] )
				{
					iSound = Clamp( V_atoi( pVisualData->GetName() + 12 ), 0, MAX_CUSTOM_WEAPON_SOUNDS - 1 );
				}

				pVisuals->aCustomWeaponSounds[ iSound ] = pVisualData->GetString();
			}
			else if ( !V_strnicmp( pVisualData->GetName(), "sound_", 6 ) )
			{
				// Advancing pointer past sound_ prefix... why couldn't they just make a subsection for sounds?
				int iSound = GetWeaponSoundFromString( pVisualData->GetName() + 6 );

				if ( iSound != -1 )
				{
					pVisuals->aWeaponSounds[ iSound ] = pVisualData->GetString();
				}
			}
			else if ( !V_stricmp( pVisualData->GetName(), "styles" ) )
			{
				for (KeyValues *pStyleData = pVisualData->GetFirstSubKey(); pStyleData != NULL; pStyleData = pStyleData->GetNextKey())
				{
					ItemStyle_t *style = new ItemStyle_t;

					GET_STRING( style, pStyleData, name );
					GET_STRING( style, pStyleData, model_player );
					GET_STRING( style, pStyleData, image_inventory );
					GET_BOOL( style, pStyleData, selectable );
					GET_INT( style, pStyleData, skin_red );
					GET_INT( style, pStyleData, skin_blu );

					for ( KeyValues *pStyleModelData = pStyleData->GetFirstSubKey(); pStyleModelData != NULL; pStyleModelData = pStyleModelData->GetNextKey() )
					{
						if ( !V_stricmp( pStyleModelData->GetName(), "model_player_per_class" ) )
						{
							GET_VALUES_FAST_STRING( style->model_player_per_class, pStyleModelData );
						}
					}

					pVisuals->styles.AddToTail( style );
				}
			}
			else if ( !V_stricmp( pVisualData->GetName(), "skin" ) )
			{
				pVisuals->skin = pVisualData->GetInt();
			}
			else if ( !V_stricmp( pVisualData->GetName(), "use_per_class_bodygroups" ) )
			{
				pVisuals->use_per_class_bodygroups = pVisualData->GetInt();
			}
			else if ( !V_stricmp( pVisualData->GetName(), "material_override" ) )
			{
				pVisuals->material_override = pVisualData->GetString();
			}
			else if ( !V_stricmp( pVisualData->GetName(), "vm_bodygroup_override" ) )
			{
				pVisuals->vm_bodygroup_override = pVisualData->GetInt();
			}
			else if ( !V_stricmp( pVisualData->GetName(), "vm_bodygroup_state_override" ) )
			{
				pVisuals->vm_bodygroup_state_override = pVisualData->GetInt();
			}
			else if ( !V_stricmp( pVisualData->GetName(), "wm_bodygroup_override" ) )
			{
				pVisuals->wm_bodygroup_override = pVisualData->GetInt();
			}
			else if ( !V_stricmp( pVisualData->GetName(), "wm_bodygroup_state_override" ) )
			{
				pVisuals->wm_bodygroup_state_override = pVisualData->GetInt();
			}
		}

		pItem->visual[ iIndex ] = pVisuals;

		return true;
	}

protected:
	void MergeDefinitionPrefabs( KeyValues *pDefinition, KeyValues *pSchemeData )
	{
		char prefab[64]{0};
		Q_snprintf( prefab, sizeof( prefab ), pSchemeData->GetString( "prefab" ) );

		if ( prefab[0] != '\0' )
		{
			//check if there's prefab for prefab.. PREFABSEPTION
			CUtlStringList strings;
			V_SplitString( prefab, " ", strings );

			FOR_EACH_VEC_BACK( strings, i )
			{
				KeyValues *pPrefabValues = NULL;
				FIND_ELEMENT( GetItemSchema()->m_PrefabsValues, strings[i], pPrefabValues );
				if ( pPrefabValues )
					MergeDefinitionPrefabs( pDefinition, pPrefabValues );
			}
		}

		InheritKVRec( pSchemeData, pDefinition );
	}

	void InheritKVRec( KeyValues *pFrom, KeyValues *pTo )
	{
		for ( KeyValues *pSubData = pFrom->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
		{
			switch ( pSubData->GetDataType() )
			{
				// Identifies the start of a subsection
				case KeyValues::TYPE_NONE:
				{
					KeyValues *pKey = pTo->FindKey( pSubData->GetName() );
					if ( pKey == NULL )
					{
						pKey = pTo->CreateNewKey();
						pKey->SetName( pSubData->GetName() );
					}

					InheritKVRec( pSubData, pKey );
					break;
				}
				// Actual types
				case KeyValues::TYPE_STRING:
				{
					pTo->SetString( pSubData->GetName(), pSubData->GetString() );
					break;
				}
				case KeyValues::TYPE_INT:
				{
					pTo->SetInt( pSubData->GetName(), pSubData->GetInt() );
					break;
				}
				case KeyValues::TYPE_FLOAT:
				{
					pTo->SetFloat( pSubData->GetName(), pSubData->GetFloat() );
					break;
				}
				case KeyValues::TYPE_WSTRING:
				{
					pTo->SetWString( pSubData->GetName(), pSubData->GetWString() );
					break;
				}
				case KeyValues::TYPE_COLOR:
				{
					pTo->SetColor( pSubData->GetName(), pSubData->GetColor() );
					break;
				}
				case KeyValues::TYPE_UINT64:
				{
					pTo->SetUint64( pSubData->GetName(), pSubData->GetUint64() );
					break;
				}
				default:
					break;
			}
		}
	}
};
CEconSchemaParser g_EconSchemaParser;


bool CEconItemDefinition::LoadFromKV( KeyValues *pDefinition )
{
	definition = pDefinition;

	GET_STRING( this, pDefinition, name );
	GET_BOOL( this, pDefinition, show_in_armory );

	GET_STRING( this, pDefinition, item_class );
	GET_STRING( this, pDefinition, item_name );
	GET_STRING( this, pDefinition, item_description );
	GET_STRING( this, pDefinition, item_type_name );

	const char *pszQuality = pDefinition->GetString( "item_quality" );
	if ( pszQuality[0] )
	{
		int iQuality = UTIL_StringFieldToInt( pszQuality, g_szQualityStrings, ARRAYSIZE( g_szQualityStrings ) );
		if ( iQuality != -1 )
		{
			item_quality = iQuality;
		}
	}

	// All items are vintage quality
	//this->item_quality = QUALITY_VINTAGE;

	GET_STRING( this, pDefinition, item_logname );
	GET_STRING( this, pDefinition, item_iconname );

	const char *pszLoadoutSlot = pDefinition->GetString( "item_slot" );

	if ( pszLoadoutSlot[0] )
	{
		item_slot = UTIL_StringFieldToInt( pszLoadoutSlot, g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );
	}

	const char *pszAnimSlot = pDefinition->GetString( "anim_slot" );
	if ( pszAnimSlot[0] )
	{
		if ( V_strcmp( pszAnimSlot, "FORCE_NOT_USED" ) != 0 )
		{
			anim_slot = UTIL_StringFieldToInt( pszAnimSlot, g_AnimSlots, TF_WPN_TYPE_COUNT );
		}
		else
		{
			anim_slot = -2;
		}
	}

	GET_BOOL( this, pDefinition, baseitem );
	GET_INT( this, pDefinition, min_ilevel );
	GET_INT( this, pDefinition, max_ilevel );
	GET_BOOL( this, pDefinition, loadondemand );

	GET_STRING( this, pDefinition, image_inventory );
	GET_INT( this, pDefinition, image_inventory_size_w );
	GET_INT( this, pDefinition, image_inventory_size_h );

	GET_STRING( this, pDefinition, model_player );
	GET_STRING( this, pDefinition, model_vision_filtered );
	GET_STRING( this, pDefinition, model_world );
	GET_STRING( this, pDefinition, extra_wearable );

	GET_INT( this, pDefinition, attach_to_hands );
	GET_INT( this, pDefinition, attach_to_hands_vm_only );
	GET_BOOL( this, pDefinition, act_as_wearable );
	GET_INT( this, pDefinition, hide_bodygroups_deployed_only );

	GET_BOOL( this, pDefinition, is_reskin );
	GET_BOOL( this, pDefinition, specialitem );
	GET_BOOL( this, pDefinition, demoknight );
	GET_STRING( this, pDefinition, holiday_restriction );
	GET_INT( this, pDefinition, year );
	GET_BOOL( this, pDefinition, is_custom_content );
	GET_BOOL( this, pDefinition, is_cut_content );
	GET_BOOL( this, pDefinition, is_multiclass_item );

	GET_STRING( this, pDefinition, item_script );

	const char *pszDropType = pDefinition->GetString("drop_type");
	if ( pszDropType[0] )
	{
		int iDropType = UTIL_StringFieldToInt( pszDropType, g_szDropTypeStrings, ARRAYSIZE( g_szDropTypeStrings ) );
		if (iDropType != -1)
		{
			drop_type = iDropType;
		}
	}


	for ( KeyValues *pSubData = pDefinition->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
	{
		if ( !V_stricmp( pSubData->GetName(), "capabilities" ) )
		{
			GET_VALUES_FAST_BOOL( capabilities, pSubData );
		}
		else if ( !V_stricmp( pSubData->GetName(), "tags" ) )
		{
			GET_VALUES_FAST_BOOL( tags, pSubData );
		}
		else if ( !V_stricmp( pSubData->GetName(), "model_player_per_class" ) )
		{
			for ( KeyValues *pClassData = pSubData->GetFirstSubKey(); pClassData != NULL; pClassData = pClassData->GetNextKey() )
			{
				const char *pszClass = pClassData->GetName();

				if ( !V_stricmp( pszClass, "basename" ) )
				{
					// Generic item, assign a model for every class.
					for ( int i = TF_FIRST_NORMAL_CLASS; i <= TF_LAST_NORMAL_CLASS; i++ )
					{
						// Add to the player model per class.
						model_player_per_class[i] = UTIL_VarArgs( pClassData->GetString(), g_aRawPlayerClassNamesShort[i] );
					}
				}
				else
				{
					// Check the class this item is for and assign it to them.
					int iClass = UTIL_StringFieldToInt( pszClass, g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );
					if ( iClass != -1 )
					{
						// Valid class, assign to it.
						model_player_per_class[iClass] = pClassData->GetString();
					}
				}
			}
		}
		else if ( !V_stricmp( pSubData->GetName(), "used_by_classes" ) )
		{
			for ( KeyValues *pClassData = pSubData->GetFirstSubKey(); pClassData != NULL; pClassData = pClassData->GetNextKey() )
			{
				const char *pszClass = pClassData->GetName();
				int iClass = UTIL_StringFieldToInt( pszClass, g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );

				if ( iClass != -1 )
				{
					used_by_classes |= ( 1 << iClass );
					const char *pszSlotname = pClassData->GetString();

					if ( pszSlotname[0] != '1' )
					{
						int iSlot = UTIL_StringFieldToInt( pszSlotname, g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );

						if ( iSlot != -1 )
							item_slot_per_class[iClass] = iSlot;
					}
				}
			}
		}
		else if ( !V_stricmp( pSubData->GetName(), "attributes" ) )
		{
			for ( KeyValues *pAttribData = pSubData->GetFirstTrueSubKey(); pAttribData != NULL; pAttribData = pAttribData->GetNextTrueSubKey() )
			{
				static_attrib_t attribute;
				if ( !attribute.BInitFromKV_MultiLine( pAttribData ) )
					continue;

				attributes.AddToTail( attribute );
			}
		}
		else if ( !V_stricmp( pSubData->GetName(), "static_attrs" ) )
		{
			for ( KeyValues *pAttribData = pSubData->GetFirstSubKey(); pAttribData != NULL; pAttribData = pAttribData->GetNextKey() )
			{
				static_attrib_t attribute;
				if ( !attribute.BInitFromKV_SingleLine( pAttribData ) )
					continue;

				attributes.AddToTail( attribute );
			}
		}
		else if ( !V_stricmp( pSubData->GetName(), "visuals_mvm_boss" ) )
		{
			// Deliberately skipping this.
		}
		else if ( !V_strnicmp( pSubData->GetName(), "visuals", 7 ) )
		{
			// Figure out what team is this meant for.
			int iVisuals = UTIL_StringFieldToInt( pSubData->GetName(), g_TeamVisualSections, TF_TEAM_COUNT );

			if ( iVisuals != -1 )
			{
				if ( iVisuals == TEAM_UNASSIGNED )
				{
					// Hacky: for standard visuals block, assign it to all teams at once.
					for ( int i = 0; i < TF_TEAM_COUNT; i++ )
					{
						if ( i == TEAM_SPECTATOR )
							continue;

						visual[i] = NULL;
						CEconSchemaParser::ParseVisuals( pSubData, this, i );
					}
				}
				else
				{
					visual[ iVisuals ] = NULL;
					CEconSchemaParser::ParseVisuals( pSubData, this, iVisuals );
				}
			}
		}
	}

	return true;
}

bool CEconAttributeDefinition::LoadFromKV( KeyValues *pDefinition )
{
	definition = pDefinition;

	V_strcpy_safe( name, pDefinition->GetString( "name", "( unnamed )" ) );
	V_strcpy_safe( attribute_class, pDefinition->GetString( "attribute_class" ) );
	V_strcpy_safe( description_string, pDefinition->GetString( "description_string" ) );

	string_attribute = ( V_stricmp( pDefinition->GetString( "attribute_type" ), "string" ) == 0 );

	const char *szFormat = pDefinition->GetString( "description_format" );
	description_format = UTIL_StringFieldToInt( szFormat, g_AttributeDescriptionFormats, ARRAYSIZE( g_AttributeDescriptionFormats ) );

	const char *szEffect = pDefinition->GetString( "effect_type" );
	effect_type = UTIL_StringFieldToInt( szEffect, g_EffectTypes, ARRAYSIZE( g_EffectTypes ) );

	const char *szType = pDefinition->GetString( "attribute_type" );
	type = GetItemSchema()->GetAttributeType( szType );

	GET_BOOL( this, pDefinition, hidden );
	GET_BOOL( this, pDefinition, stored_as_integer );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
template<typename T>
class ISchemaAttributeTypeBase : public ISchemaAttributeType
{
public:
	virtual void InitializeNewEconAttributeValue( attrib_data_union_t *pValue ) const
	{
		if ( sizeof( T ) <= sizeof( uint32 ) )
			new( &pValue->iVal ) T;
		else if ( sizeof( T ) <= sizeof( void * ) )
			new( &pValue->bVal ) T;
		else
			pValue->bVal = (byte *)new T;
	}

	virtual void UnloadEconAttributeValue( attrib_data_union_t *pValue ) const
	{
		if ( sizeof( T ) <= sizeof( uint32 ) )
			reinterpret_cast<T *>( &pValue->iVal )->~T();
		else if ( sizeof( T ) <= sizeof( void * ) )
			reinterpret_cast<T *>( &pValue->bVal )->~T();
		else
			delete pValue->bVal;
	}

	virtual bool OnIterateAttributeValue( IEconAttributeIterator *pIterator, const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value ) const
	{
		return pIterator->OnIterateAttributeValue( pAttrDef, GetTypedDataFromValue( value ) );
	}

private:
	T const &GetTypedDataFromValue( const attrib_data_union_t &value ) const
	{
		if ( sizeof( T ) <= sizeof( uint32 ) )
			return *reinterpret_cast<const T *>( &value.iVal );

		if ( sizeof( T ) <= sizeof( void* ) )
			return *reinterpret_cast<const T *>( &value.bVal );

		Assert( value.bVal );
		return *reinterpret_cast<const T *>( value.bVal );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSchemaAttributeType_Default : public ISchemaAttributeTypeBase<unsigned int>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		if ( bUnk )
		{
			double val = 0.0;
			if ( pString && pString[0] )
				val = strtod( pString, NULL );

			pValue->flVal = val;
		}
		else if ( pAttrDef->stored_as_integer )
		{
			pValue->iVal = V_atoi64( pString );
		}
		else
		{
			pValue->flVal = V_atof( pString );
		}

		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, char *pString ) const
	{
		if ( pAttrDef->stored_as_integer )
		{
			V_strcpy( pString, UTIL_VarArgs( "%u", value.iVal ) );
		}
		else
		{
			V_strcpy( pString, UTIL_VarArgs( "%f", value.flVal ) );
		}
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSchemaAttributeType_UInt64 : public ISchemaAttributeTypeBase<unsigned long long>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		*(pValue->lVal) = V_atoui64( pString );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, char *pString ) const
	{
		V_strcpy( pString, UTIL_VarArgs( "%llu", *(value.lVal) ) );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSchemaAttributeType_Float : public ISchemaAttributeTypeBase<float>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		pValue->flVal = V_atof( pString );
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, char *pString ) const
	{
		V_strcpy( pString, UTIL_VarArgs( "%f", value.flVal ) );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSchemaAttributeType_String : public ISchemaAttributeTypeBase<CAttribute_String>
{
public:
	virtual bool BConvertStringToEconAttributeValue( const CEconAttributeDefinition *pAttrDef, const char *pString, attrib_data_union_t *pValue, bool bUnk = true ) const
	{
		*(pValue->sVal) = pString;
		return true;
	}

	virtual void ConvertEconAttributeValueToString( const CEconAttributeDefinition *pAttrDef, const attrib_data_union_t &value, char *pString ) const
	{
		if ( pAttrDef->string_attribute )
		{
			V_strcpy( pString, *(value.sVal) );
		}
	}
};

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CEconItemSchema::CEconItemSchema() :
	m_Items( DefLessFunc(uint32) ),
	m_Attributes( DefLessFunc(uint32) ),
	m_bInited( false )
{
}

CEconItemSchema::~CEconItemSchema()
{
	m_Items.PurgeAndDeleteElements();
	m_Attributes.PurgeAndDeleteElements();

	FOR_EACH_DICT_FAST( m_PrefabsValues, i )
	{
		m_PrefabsValues[i]->deleteThis();
	}

	FOR_EACH_VEC_BACK( m_AttributeTypes, i )
	{
		delete m_AttributeTypes[i].pType;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initializer
//-----------------------------------------------------------------------------
bool CEconItemSchema::Init( void )
{
	if ( !m_bInited )
	{
		// Must register activities early so we can parse animation replacements.
		ActivityList_Free();
		ActivityList_RegisterSharedActivities();

		InitAttributeTypes();

		float flStartTime = engine->Time();
		g_EconSchemaParser.InitParser( items_game, true, false );
		float flEndTime = engine->Time();
		Msg( "Processing item schema took %.02fms. Parsed %d items and %d attributes.\n", ( flEndTime - flStartTime ) * 1000.0f, m_Items.Count(), m_Attributes.Count() );

		m_bInited = true;
	}
	
	return true;
}

void CEconItemSchema::InitAttributeTypes( void )
{
	FOR_EACH_VEC( m_AttributeTypes, i )
	{
		delete m_AttributeTypes[i].pType;
	}
	m_AttributeTypes.Purge();

	attr_type_t defaultType;
	defaultType.szName = NULL;
	defaultType.pType = new CSchemaAttributeType_Default;
	m_AttributeTypes.AddToTail( defaultType );

	attr_type_t longType;
	longType.szName = "uint64";
	longType.pType = new CSchemaAttributeType_UInt64;
	m_AttributeTypes.AddToTail( longType );

	attr_type_t floatType;
	floatType.szName = "float";
	floatType.pType = new CSchemaAttributeType_Float;
	m_AttributeTypes.AddToTail( floatType );

	attr_type_t stringType;
	stringType.szName = "string";
	stringType.pType = new CSchemaAttributeType_String;
	m_AttributeTypes.AddToTail( stringType );
}

bool CEconItemSchema::LoadFromFile( void )
{
	Reset();

	KeyValuesAD schema("KVDataFile");
	if ( !schema->LoadFromFile( g_pFullFileSystem, items_game ) )
		return false;

	ParseSchema( schema );

	return true;
}

bool CEconItemSchema::LoadFromBuffer( CUtlBuffer &buf )
{
	Reset();

	KeyValuesAD schema("KVDataFile");
	if ( !schema->ReadAsBinary( buf ) )
		return false;

	ParseSchema( schema );

	return true;
}

bool CEconItemSchema::SaveToBuffer( CUtlBuffer &buf )
{
	m_pSchema->RecursiveSaveToFile( buf, 0 );
	return true;
}

void CEconItemSchema::Reset( void )
{
	m_GameInfo.Purge();
	m_Colors.Purge();
	m_Qualities.Purge();

	m_Items.PurgeAndDeleteElements();
	m_Attributes.PurgeAndDeleteElements();

	FOR_EACH_DICT_FAST( m_PrefabsValues, i )
	{
		m_PrefabsValues[i]->deleteThis();
	}
	m_PrefabsValues.Purge();

	m_unSchemaResetCount++;
}

void CEconItemSchema::Precache( void )
{
	static CSchemaAttributeHandle pAttribDef_CustomProjectile( "custom projectile model" );

	// Precache everything from schema.
	FOR_EACH_MAP( m_Items, i )
	{
		CEconItemDefinition *pItem = m_Items[i];
		
		// Precache models.
		if ( pItem->GetWorldModel() )
			CBaseEntity::PrecacheModel( pItem->GetWorldModel() );

		if ( pItem->GetPlayerModel() )
			CBaseEntity::PrecacheModel( pItem->GetPlayerModel() );

		for ( int iClass = 0; iClass < TF_CLASS_COUNT_ALL; iClass++ )
		{
			if ( pItem->GetPerClassModel( iClass ) )
				CBaseEntity::PrecacheModel( pItem->GetPerClassModel( iClass ) );
		}

		if ( pItem->GetExtraWearableModel() )
			CBaseEntity::PrecacheModel( pItem->GetExtraWearableModel() );
		
		// Precache visuals.
		for ( int i = TEAM_UNASSIGNED; i < TF_TEAM_COUNT; i++ )
		{
			if ( i == TEAM_SPECTATOR )
				continue;

			PerTeamVisuals_t *pVisuals = pItem->GetVisuals( i );
			if ( pVisuals == NULL )
				continue;

			// Precache sounds.
			for ( int i = 0; i < NUM_SHOOT_SOUND_TYPES; i++ )
			{
				if ( pVisuals->GetWeaponShootSound( i ) )
					CBaseEntity::PrecacheScriptSound( pVisuals->GetWeaponShootSound( i ) );
			}
			for ( int i = 0; i < MAX_CUSTOM_WEAPON_SOUNDS; i++ )
			{
				if ( pVisuals->GetCustomWeaponSound( i ) )
					CBaseEntity::PrecacheScriptSound( pVisuals->GetCustomWeaponSound( i ) );
			}

			// Precache attachments.
			for ( int i = 0; i < pVisuals->attached_models.Count(); i++ )
			{
				const char *pszModel = pVisuals->attached_models[i].model;
				if ( pszModel && pszModel[0] != '\0' )
					CBaseEntity::PrecacheModel( pszModel );
			}

			// Precache particles
			// Custom Particles
			if ( pVisuals->GetCustomParticleSystem() )
			{
				PrecacheParticleSystem( pVisuals->GetCustomParticleSystem() );
			}
			// Muzzle Flash
			if ( pVisuals->GetMuzzleFlash() )
			{
				PrecacheParticleSystem( pVisuals->GetMuzzleFlash() );
			}
			// Tracer Effect
			if ( pVisuals->GetTracerFX() )
			{
				PrecacheParticleSystem( pVisuals->GetTracerFX() );
			}

		}

		// Cache all attrbute names.
		for ( static_attrib_t const &attrib : pItem->attributes )
		{
			const CEconAttributeDefinition *pAttribute = attrib.GetStaticData();

			// Special case for custom_projectile_model attribute.
			if ( pAttribute == pAttribDef_CustomProjectile )
			{
				CBaseEntity::PrecacheModel( attrib.value );
			}
		}
	}
}

void CEconItemSchema::ParseSchema( KeyValues *pKVData )
{
	g_EconSchemaParser.Parse( pKVData );
}

CEconItemDefinition *CEconItemSchema::GetItemDefinition( int id )
{
	if ( id < 0 )
		return NULL;
	CEconItemDefinition *itemdef = NULL;
	FIND_ELEMENT( m_Items, id, itemdef );
	return itemdef;
}

CEconItemDefinition *CEconItemSchema::GetItemDefinitionByName( const char *name )
{
	FOR_EACH_MAP_FAST( m_Items, i )
	{
		if ( !V_stricmp( m_Items[i]->GetName(), name ) )
		{
			return m_Items[i];
		}
	}

	return NULL;
}

CEconAttributeDefinition *CEconItemSchema::GetAttributeDefinition( int id )
{
	if ( id < 0 )
		return NULL;
	CEconAttributeDefinition *itemdef = NULL;
	FIND_ELEMENT( m_Attributes, id, itemdef );
	return itemdef;
}

CEconAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByName( const char *name )
{
	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->GetName(), name ) )
		{
			return m_Attributes[i];
		}
	}

	return NULL;
}

CEconAttributeDefinition *CEconItemSchema::GetAttributeDefinitionByClass( const char *classname )
{
	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->GetClassName(), classname ) )
		{
			return m_Attributes[i];
		}
	}

	return NULL;
}

ISchemaAttributeType *CEconItemSchema::GetAttributeType( const char *name ) const
{
	FOR_EACH_VEC( m_AttributeTypes, i )
	{
		if ( m_AttributeTypes[i].szName == name )
			return m_AttributeTypes[i].pType;
	}

	return NULL;
}


