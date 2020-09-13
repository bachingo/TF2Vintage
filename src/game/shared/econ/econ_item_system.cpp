#include "cbase.h"
#include "econ_item_system.h"
#include "script_parser.h"
#include "activitylist.h"
#include "attribute_types.h"

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

const char *g_LoadoutDropTypes[] =
{
	"none",
	"drop",
	"break",
};

//-----------------------------------------------------------------------------
// Purpose: for the UtlMap
//-----------------------------------------------------------------------------
static bool schemaLessFunc( const int &lhs, const int &rhs )
{
	return lhs < rhs;
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

	void Parse( KeyValues *pKeyValuesData, bool bWildcard, const char *szFileWithoutEXT )
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
		for ( KeyValues *pData = pKeyValuesData->GetFirstTrueSubKey(); pData != NULL; pData = pData->GetNextTrueSubKey() )
		{
			// Skip over default item, not sure why it's there.
			if ( V_stricmp( pData->GetName(), "default" ) == 0 )
				continue;

			CEconItemDefinition *pItem = GetItemSchema()->CreateNewItemDefinition();
			int index = atoi( pData->GetName() );
			pItem->index = index;

			KeyValues *pDefinition = new KeyValues( pData->GetName() );
			MergeDefinitionPrefabs( pDefinition, pData );
			pItem->definition = pDefinition;

			GET_STRING( pItem, pDefinition, name );
			GET_BOOL( pItem, pDefinition, show_in_armory );

			GET_STRING( pItem, pDefinition, item_class );
			GET_STRING( pItem, pDefinition, item_name );
			GET_STRING( pItem, pDefinition, item_description );
			GET_STRING( pItem, pDefinition, item_type_name );

			const char *pszQuality = pDefinition->GetString( "item_quality" );
			if ( pszQuality[0] )
			{
				int iQuality = UTIL_StringFieldToInt( pszQuality, g_szQualityStrings, ARRAYSIZE( g_szQualityStrings ) );
				if ( iQuality != -1 )
				{
					pItem->item_quality = iQuality;
				}
			}

			// All items are vintage quality
			//pItem->item_quality = QUALITY_VINTAGE;

			GET_STRING( pItem, pDefinition, item_logname );
			GET_STRING( pItem, pDefinition, item_iconname );

			const char *pszLoadoutSlot = pDefinition->GetString( "item_slot" );

			if ( pszLoadoutSlot[0] )
			{
				pItem->item_slot = UTIL_StringFieldToInt( pszLoadoutSlot, g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );
			}

			const char *pszAnimSlot = pDefinition->GetString( "anim_slot" );
			if ( pszAnimSlot[0] )
			{
				if ( V_strcmp( pszAnimSlot, "FORCE_NOT_USED" ) != 0 )
				{
					pItem->anim_slot = UTIL_StringFieldToInt( pszAnimSlot, g_AnimSlots, TF_WPN_TYPE_COUNT );
				}
				else
				{
					pItem->anim_slot = -2;
				}
			}

			GET_BOOL( pItem, pDefinition, baseitem );
			GET_INT( pItem, pDefinition, min_ilevel );
			GET_INT( pItem, pDefinition, max_ilevel );
			GET_BOOL( pItem, pDefinition, loadondemand );

			GET_STRING( pItem, pDefinition, image_inventory );
			GET_INT( pItem, pDefinition, image_inventory_size_w );
			GET_INT( pItem, pDefinition, image_inventory_size_h );

			GET_STRING( pItem, pDefinition, model_player );
			GET_STRING( pItem, pDefinition, model_vision_filtered );
			GET_STRING( pItem, pDefinition, model_world );
			GET_STRING( pItem, pDefinition, extra_wearable );

			GET_INT( pItem, pDefinition, attach_to_hands );
			GET_INT( pItem, pDefinition, attach_to_hands_vm_only );
			GET_BOOL( pItem, pDefinition, act_as_wearable );
			GET_INT( pItem, pDefinition, hide_bodygroups_deployed_only );

			GET_BOOL( pItem, pDefinition, is_reskin );
			GET_BOOL( pItem, pDefinition, specialitem );
			GET_BOOL( pItem, pDefinition, demoknight );
			GET_STRING( pItem, pDefinition, holiday_restriction );
			GET_INT( pItem, pDefinition, year );
			GET_BOOL( pItem, pDefinition, is_custom_content );
			GET_BOOL( pItem, pDefinition, is_cut_content );
			GET_BOOL( pItem, pDefinition, is_multiclass_item );
			
			const char *pszDropType = pDefinition->GetString("drop_type");
			if ( pszDropType[0] )
			{
				int iDropType = UTIL_StringFieldToInt( pszDropType, g_LoadoutDropTypes, ARRAYSIZE( g_LoadoutDropTypes ) );
				if (iDropType != -1)
				{
					pItem->drop_type = iDropType;
				}
			}


			for ( KeyValues *pSubData = pDefinition->GetFirstSubKey(); pSubData != NULL; pSubData = pSubData->GetNextKey() )
			{
				if ( !V_stricmp( pSubData->GetName(), "capabilities" ) )
				{
					GET_VALUES_FAST_BOOL( pItem->capabilities, pSubData );
				}
				else if ( !V_stricmp( pSubData->GetName(), "tags" ) )
				{
					GET_VALUES_FAST_BOOL( pItem->tags, pSubData );
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
								pItem->model_player_per_class[i] = UTIL_VarArgs( pClassData->GetString(), g_aRawPlayerClassNamesShort[i] );
							}
						}
						else
						{
							// Check the class this item is for and assign it to them.
							int iClass = UTIL_StringFieldToInt( pszClass, g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );
							if ( iClass != -1 )
							{
								// Valid class, assign to it.
								pItem->model_player_per_class[iClass] = pClassData->GetString();
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
							pItem->used_by_classes |= ( 1 << iClass );
							const char *pszSlotname = pClassData->GetString();

							if ( pszSlotname[0] != '1' )
							{
								int iSlot = UTIL_StringFieldToInt( pszSlotname, g_LoadoutSlots, TF_LOADOUT_SLOT_COUNT );

								if ( iSlot != -1 )
									pItem->item_slot_per_class[iClass] = iSlot;
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

						pItem->attributes.AddToTail( attribute );
					}
				}
				else if ( !V_stricmp( pSubData->GetName(), "static_attrs" ) )
				{
					for ( KeyValues *pAttribData = pSubData->GetFirstSubKey(); pAttribData != NULL; pAttribData = pAttribData->GetNextKey() )
					{
						static_attrib_t attribute;
						if ( !attribute.BInitFromKV_SingleLine( pAttribData ) )
							continue;

						pItem->attributes.AddToTail( attribute );
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

								pItem->visual[i] = NULL;
								ParseVisuals( pSubData, pItem, i );
							}
						}
						else
						{
							pItem->visual[ iVisuals ] = NULL;
							ParseVisuals( pSubData, pItem, iVisuals );
						}
					}
				}
			}

			GetItemSchema()->m_Items.Insert( index, pItem );
		}
	};

	void ParseAttributes( KeyValues *pKeyValuesData )
	{
		for ( KeyValues *pSubData = pKeyValuesData->GetFirstTrueSubKey(); pSubData != NULL; pSubData = pSubData->GetNextTrueSubKey() )
		{
			CEconAttributeDefinition *pAttribute = GetItemSchema()->CreateNewAttribDefinition();
			pAttribute->index = V_atoi( pSubData->GetName() );

			V_strcpy_safe( pAttribute->name, pSubData->GetString( "name", "( unnamed )" ) );
			V_strcpy_safe( pAttribute->attribute_class, pSubData->GetString( "attribute_class" ) );
			V_strcpy_safe( pAttribute->description_string, pSubData->GetString( "description_string" ) );

			pAttribute->string_attribute = ( V_stricmp( pSubData->GetString( "attribute_type" ), "string" ) == 0 );

			const char *szFormat = pSubData->GetString( "description_format" );
			pAttribute->description_format = UTIL_StringFieldToInt( szFormat, g_AttributeDescriptionFormats, ARRAYSIZE( g_AttributeDescriptionFormats ) );

			const char *szEffect = pSubData->GetString( "effect_type" );
			pAttribute->effect_type = UTIL_StringFieldToInt( szEffect, g_EffectTypes, ARRAYSIZE( g_EffectTypes ) );

			const char *szType = pSubData->GetString( "attribute_type" );
			pAttribute->type = GetItemSchema()->GetAttributeType( szType );

			GET_BOOL( pAttribute, pSubData, hidden );
			GET_BOOL( pAttribute, pSubData, stored_as_integer );

			pAttribute->definition = pSubData->MakeCopy();

			GetItemSchema()->m_Attributes.Insert( pAttribute->index, pAttribute );
		}
	};

	bool ParseVisuals( KeyValues *pData, CEconItemDefinition *pItem, int iIndex )
	{
		PerTeamVisuals_t *pVisuals = new PerTeamVisuals_t;

		for ( KeyValues *pVisualData = pData->GetFirstSubKey(); pVisualData != NULL; pVisualData = pVisualData->GetNextKey() )
		{
			if ( !V_stricmp( pVisualData->GetName(), "player_bodygroups" ) )
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
				GET_VALUES_FAST_STRING( pVisuals->playback_activity, pVisualData );
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
					ItemStyle_t *style;
					IF_ELEMENT_FOUND( pVisuals->styles, pStyleData->GetName() )
					{
						style = pVisuals->styles.Element( index );
					}
					else
					{
						style = new ItemStyle_t;
						pVisuals->styles.Insert( pStyleData->GetName(), style );
					}

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
			else
			{
				GET_VALUES_FAST_STRING( pVisuals->misc_info, pVisualData );
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

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CEconItemSchema::CEconItemSchema()
{
	m_Items.SetLessFunc( schemaLessFunc );
	m_Attributes.SetLessFunc( schemaLessFunc );

	m_bInited = false;
}

CEconItemSchema::~CEconItemSchema()
{
	FOR_EACH_DICT_FAST( m_PrefabsValues, i )
	{
		m_PrefabsValues[i]->deleteThis();
	}
	m_PrefabsValues.RemoveAll();

	m_Items.PurgeAndDeleteElements();
	m_Attributes.PurgeAndDeleteElements();

	for ( attr_type_t const &atype : m_AttributeTypes )
	{
		delete atype.pType;
	}
	m_AttributeTypes.RemoveAll();
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
		g_EconSchemaParser.InitParser( "scripts/items/items_game.txt", true, false );
		float flEndTime = engine->Time();
		Msg( "Processing item schema took %.02fms. Parsed %d items and %d attributes.\n", ( flEndTime - flStartTime ) * 1000.0f, m_Items.Count(), m_Attributes.Count() );

		m_bInited = true;
	}

	return true;
}

void CEconItemSchema::InitAttributeTypes( void )
{
	attr_type_t defaultType;
	defaultType.szName = NULL;
	defaultType.pType = new CSchemaAttributeType_Default;
	m_AttributeTypes[ m_AttributeTypes.AddToTail() ] = defaultType;

	attr_type_t longType;
	longType.szName = "uint64";
	longType.pType = new CSchemaAttributeType_UInt64;
	m_AttributeTypes[ m_AttributeTypes.AddToTail() ] = longType;

	attr_type_t floatType;
	floatType.szName = "float";
	floatType.pType = new CSchemaAttributeType_Float;
	m_AttributeTypes[ m_AttributeTypes.AddToTail() ] = floatType;

	attr_type_t stringType;
	stringType.szName = "string";
	stringType.pType = new CSchemaAttributeType_String;
	m_AttributeTypes[ m_AttributeTypes.AddToTail() ] = stringType;
}

void CEconItemSchema::Precache( void )
{
	static CSchemaAttributeHandle pAttribDef_CustomProjectile( "custom projectile model" );

	// Precache everything from schema.
	FOR_EACH_MAP( m_Items, i )
	{
		CEconItemDefinition *pItem = m_Items[i];
		
		// Precache models.
		if ( pItem->model_world && pItem->model_world[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->model_world );

		if ( pItem->model_player && pItem->model_player[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->model_player );

		for ( int iClass = 0; iClass < TF_CLASS_COUNT_ALL; iClass++ )
		{
			const char *pszModel = pItem->model_player_per_class[iClass];
			if ( pszModel && pszModel[0] != '\0' )
				CBaseEntity::PrecacheModel( pszModel );
		}

		if ( pItem->extra_wearable && pItem->extra_wearable[0] != '\0' )
			CBaseEntity::PrecacheModel( pItem->extra_wearable );
		
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
				if ( pVisuals->aWeaponSounds[i] && pVisuals->aWeaponSounds[i][0] != '\0' )
					CBaseEntity::PrecacheScriptSound( pVisuals->aWeaponSounds[i] );
			}
			for ( int i = 0; i < MAX_CUSTOM_WEAPON_SOUNDS; i++ )
			{
				if ( pVisuals->aCustomWeaponSounds[i] && pVisuals->aCustomWeaponSounds[i][0] != '\0' )
					CBaseEntity::PrecacheScriptSound( pVisuals->aCustomWeaponSounds[i] );
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
			const char *pszParticle = pVisuals->custom_particlesystem;
			if ( pszParticle && pszParticle[0] != '\0' )
			{
				PrecacheParticleSystem( pszParticle );
			}
			// Muzzle Flash
			const char *pszMuzzleFlash = pVisuals->muzzle_flash;
			if ( pszMuzzleFlash && pszMuzzleFlash[0] != '\0' )
			{
				PrecacheParticleSystem( pszMuzzleFlash );
			}
			// Tracer Effect
			const char *pszTracerEffect = pVisuals->tracer_effect;
			if ( pszTracerEffect && pszTracerEffect[0] != '\0' )
			{
				PrecacheParticleSystem( pszTracerEffect );
			}

		}

		// Cache all attrbute names.
		for ( static_attrib_t const &attrib : pItem->attributes )
		{
			const CEconAttributeDefinition *pAttribute = attrib.GetStaticData();

			// Special case for custom_projectile_model attribute.
			if ( pAttribute == pAttribDef_CustomProjectile )
			{
				CBaseEntity::PrecacheModel( attrib.value.sVal->Get() );
			}
		}
	}
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
		Assert( m_Items[i]->index > -1 && m_Items[i]->name );
		if ( m_Items[i]->index > -1 && !V_stricmp( m_Items[i]->name, name ) )
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
		Assert( m_Attributes[i]->name[0] );
		if ( !V_stricmp( m_Attributes[i]->name, name ) )
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
		Assert( m_Attributes[i]->attribute_class[0] );
		if ( !V_stricmp( m_Attributes[i]->attribute_class, classname ) )
		{
			return m_Attributes[i];
		}
	}

	return NULL;
}

int CEconItemSchema::GetAttributeIndex( const char *name )
{
	if ( !name )
		return -1;

	FOR_EACH_MAP_FAST( m_Attributes, i )
	{
		if ( !V_stricmp( m_Attributes[i]->name, name ) )
		{
			return m_Attributes.Key( i );
		}
	}

	return -1;
}

ISchemaAttributeType *CEconItemSchema::GetAttributeType( const char *name ) const
{
	for ( attr_type_t const &type : m_AttributeTypes )
	{
		if ( type.szName == name )
			return type.pType;
	}

	return nullptr;
}

CEconItemDefinition *CEconItemSchema::CreateNewItemDefinition( void )
{
	return new CEconItemDefinition;
}

CEconAttributeDefinition *CEconItemSchema::CreateNewAttribDefinition( void )
{
	return new CEconAttributeDefinition;
}
