#ifndef ECON_ITEM_SCHEMA_H
#define ECON_ITEM_SCHEMA_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
#include "gamestringpool.h"
#include "econ_item_system.h"

class IEconAttributeIterator;

enum
{
	ATTRIB_FORMAT_INVALID = -1,
	ATTRIB_FORMAT_PERCENTAGE = 0,
	ATTRIB_FORMAT_INVERTED_PERCENTAGE,
	ATTRIB_FORMAT_ADDITIVE,
	ATTRIB_FORMAT_ADDITIVE_PERCENTAGE,
	ATTRIB_FORMAT_OR,
	ATTRIB_FORMAT_DATE,
	ATTRIB_FORMAT_ACCOUNT_ID,
	ATTRIB_FORMAT_PARTICLE_INDEX,
	ATTRIB_FORMAT_KILLSTREAKEFFECT_INDEX,
	ATTRIB_FORMAT_KILLSTREAK_IDLEEFFECT_INDEX,
	ATTRIB_FORMAT_ITEM_DEF,
	ATTRIB_FORMAT_FROM_LOOKUP_TABLE
};
	
enum
{
	ATTRIB_EFFECT_INVALID = -1,
	ATTRIB_EFFECT_UNUSUAL = 0,
	ATTRIB_EFFECT_STRANGE,
	ATTRIB_EFFECT_NEUTRAL,
	ATTRIB_EFFECT_POSITIVE,
	ATTRIB_EFFECT_NEGATIVE,
};

enum
{
	ATTRTYPE_INVALID = -1,
	ATTRTYPE_INT,
	ATTRTYPE_UINT64,
	ATTRTYPE_FLOAT,
	ATTRTYPE_STRING
};

enum
{
	QUALITY_NORMAL,
	QUALITY_GENUINE,
	QUALITY_RARITY2,
	QUALITY_VINTAGE,
	QUALITY_RARITY3,
	QUALITY_UNUSUAL,
	QUALITY_UNIQUE,
	QUALITY_COMMUNITY,
	QUALITY_VALVE,
	QUALITY_SELFMADE,
	QUALITY_CUSTOMIZED,
	QUALITY_STRANGE,
	QUALITY_COMPLETED,
	QUALITY_HUNTED,
	QUALITY_COLLECTOR,
	QUALITY_DECORATED,
};

enum
{
	DROPTYPE_NULL, // same as none
	DROPTYPE_NONE,
	DROPTYPE_DROP,
	DROPTYPE_BREAK,
};

extern const char *g_szQualityColorStrings[];
extern const char *g_szQualityLocalizationStrings[];

#define CLEAR_STR(name) \
		name = NULL


class CAttribute_String
{
	DECLARE_CLASS_NOBASE( CAttribute_String );
public:

	CAttribute_String();
	// Explicitly copy construct
	EXPLICIT CAttribute_String( CAttribute_String const &src );
	virtual ~CAttribute_String();

	FORCEINLINE void Initialize( void )
	{
		if ( m_bInitialized )
			Clear();

		m_bInitialized = true;
	}

	void CopyFrom( CAttribute_String const &src )
	{
		Clear();

		*this = src;
	}

	void Assign( char const *src )
	{
		Assert( !m_bInitialized );

		Initialize();
		m_pString.Set( src );
		m_nLength = V_strlen( src );
	}

	void Clear( void )
	{
		if ( m_bInitialized )
			m_pString.Clear();

		m_bInitialized = false;
	}

	CAttribute_String &operator=( CAttribute_String const &src );
	CAttribute_String &operator=( char const *src );

#if defined GAME_DLL
	CAttribute_String &operator=( string_t const &src )
	{
		return this->operator=( STRING( src ) );
	}
#endif

	// Inner string access
	const char *Get( void ) const { return m_pString.Get(); }
	CUtlConstString *GetForModify( void ) { return &m_pString; }
	size_t Length() const { return m_nLength; }

	operator char const *( ) { return Get(); }
	operator char const *( ) const { return Get(); }

#if defined GAME_DLL
	operator string_t ( ) { return MAKE_STRING( Get() ); }
	operator string_t ( ) const { return MAKE_STRING( Get() ); }
#endif

private:
	CUtlConstString m_pString;
	int m_nLength;
	bool m_bInitialized;
};

typedef struct EconQuality
{
	EconQuality()
	{
		value = 0;
	}

	int value;
} Quality_t;

typedef struct EconColor
{
	EconColor()
	{
		CLEAR_STR( color_name );
	}

	char const *color_name;
} Color_t;

typedef enum
{
	WEARABLEANIM_EQUIP,
	WEARABLEANIM_STARTBUILDING,
	WEARABLEANIM_STOPBUILDING,
	WEARABLEANIM_STARTTAUNTING,
	WEARABLEANIM_STOPTAUNTING,
	NUM_WEARABLEANIM_TYPES
} wearableanimplayback_t;

typedef struct
{
	wearableanimplayback_t playback;
	int activity;
	char const *activity_name;
} activity_on_wearable_t;

// Attached Models
#define AM_WORLDMODEL	(1 << 0)
#define AM_VIEWMODEL	(1 << 1)
struct AttachedModel_t
{
	char model[128];
	int  model_display_flags;
};

typedef union
{
	uint32 iVal;
	float flVal;
	uint64 *lVal;
	CAttribute_String *sVal;
	byte *bVal;

	operator uint64 const &( ) const { return *lVal; }
	operator CAttribute_String const &( ) const { return *sVal; }
	operator char const *( ) const { return *sVal; }
} attrib_data_union_t;
static_assert( sizeof( attrib_data_union_t ) == 4, "If the size changes you've done something wrong!" );

typedef struct static_attrib_s
{
	CEconAttributeDefinition const *GetStaticData( void ) const;
	ISchemaAttributeType const *GetAttributeType( void ) const;
	bool BInitFromKV_SingleLine( KeyValues *const kv );
	bool BInitFromKV_MultiLine( KeyValues *const kv );

	static_attrib_s()
	{
		iAttribIndex = 0;
		value.iVal = 0;
	}

	static_attrib_s( const static_attrib_s &rhs )
	{
		iAttribIndex = rhs.iAttribIndex;
		value = rhs.value;
	}

	attrib_def_index_t iAttribIndex;
	attrib_data_union_t value;
} static_attrib_t;

typedef struct EconItemStyle
{
	EconItemStyle()
	{
		CLEAR_STR( name );
		CLEAR_STR( model_player );
		CLEAR_STR( image_inventory );
		skin_red = 0;
		skin_blu = 0;
		selectable = false;
	}
	~EconItemStyle()
	{
		model_player_per_class.PurgeAndDeleteElements();
	}

private:
	char const *name;
	char const *model_player;
	char const *image_inventory;

public:
	int skin_red;
	int skin_blu;
	bool selectable;
	CUtlDict< const char*, unsigned short > model_player_per_class;

	friend class CEconItemDefinition;
} ItemStyle_t;

#define MAX_CUSTOM_WEAPON_SOUNDS   10
typedef struct EconPerTeamVisuals
{
	EconPerTeamVisuals()
	{
		animation_replacement.SetLessFunc( DefLessFunc( int ) );
		player_bodygroups.SetLessFunc( StringLessThan );
		V_memset( aCustomWeaponSounds, 0, sizeof( aCustomWeaponSounds ) );
		V_memset( aWeaponSounds, 0, sizeof( aWeaponSounds ) );
		CLEAR_STR( custom_particlesystem );
		CLEAR_STR( muzzle_flash );
		CLEAR_STR( tracer_effect );
		CLEAR_STR( material_override );
		skin = -1;
		use_per_class_bodygroups = 0;
		vm_bodygroup_override = -1;
		vm_bodygroup_state_override = -1;
		wm_bodygroup_override = -1;
		wm_bodygroup_state_override = -1;
	}
	~EconPerTeamVisuals()
	{
		styles.PurgeAndDeleteElements();
	}

	char const *GetWeaponShootSound( int sound )
	{
		Assert( sound >= 0 && sound < NUM_SHOOT_SOUND_TYPES );
		if ( aWeaponSounds[sound] && aWeaponSounds[sound][0] != '\0' )
			return aWeaponSounds[sound];

		return NULL;
	}
	char const *GetCustomWeaponSound( int sound )
	{
		Assert( sound >= 0 && sound < MAX_CUSTOM_WEAPON_SOUNDS );
		if ( aCustomWeaponSounds[sound] && aCustomWeaponSounds[sound][0] != '\0' )
			return aCustomWeaponSounds[sound];

		return NULL;
	}
	char const *GetMuzzleFlash( void ) const
	{
		if ( muzzle_flash && muzzle_flash[0] != '\0' )
			return muzzle_flash;

		return NULL;
	}
	char const *GetTracerFX( void ) const
	{
		if ( tracer_effect && tracer_effect[0] != '\0' )
			return tracer_effect;

		return NULL;
	}
	char const *GetMaterialOverride( void ) const
	{
		if ( material_override && material_override[0] != '\0' )
			return material_override;

		return NULL;
	}
	char const *GetCustomParticleSystem( void ) const
	{
		if ( custom_particlesystem && custom_particlesystem[0] != '\0' )
			return custom_particlesystem;

		return NULL;
	}

	void operator=( EconPerTeamVisuals const &src );

private:
	char const *aCustomWeaponSounds[ MAX_CUSTOM_WEAPON_SOUNDS ];
	char const *aWeaponSounds[ NUM_SHOOT_SOUND_TYPES ];
	char const *custom_particlesystem;
	char const *muzzle_flash;
	char const *tracer_effect;
	char const *material_override;

public:
	CUtlMap< const char*, int > player_bodygroups;
	CUtlMap< int, int > animation_replacement;
	CUtlVector< activity_on_wearable_t > playback_activity;
	CUtlVector< AttachedModel_t > attached_models;
	CUtlVector< ItemStyle_t* > styles;
	int skin;
	int use_per_class_bodygroups;
	int vm_bodygroup_override;
	int vm_bodygroup_state_override;
	int wm_bodygroup_override;
	int wm_bodygroup_state_override;

	friend class CEconItemDefinition;
} PerTeamVisuals_t;

class CEconAttributeDefinition
{
public:
	CEconAttributeDefinition()
	{
		definition = NULL;
		index = INVALID_ATTRIBUTE_DEF_INDEX;
		CLEAR_STR( name );
		CLEAR_STR( attribute_class );
		CLEAR_STR( description_string );
		string_attribute = false;
		description_format = -1;
		hidden = false;
		effect_type = -1;
		stored_as_integer = false;
		iszAttrClass = NULL_STRING;
	}
	~CEconAttributeDefinition()
	{
		if( definition )
			definition->deleteThis();
	}

	KeyValues *GetStaticDefinition( void ) const { return definition; }

	char const *GetName( void ) const
	{
		Assert( name && name[0] );
		return name;
	}
	char const *GetClassName( void ) const
	{
		Assert( attribute_class && attribute_class[0] );
		return attribute_class;
	}
	char const *GetDescription( void ) const
	{
		Assert( description_string && description_string[0] );
		return description_string;
	}

	string_t GetCachedClass( void ) const
	{
		if ( !iszAttrClass && attribute_class )
			iszAttrClass = AllocPooledString( attribute_class );

		return iszAttrClass;
	}

	bool LoadFromKV( KeyValues *pKV );

private:
	char const *name;
	char const *attribute_class;
	char const *description_string;

	mutable string_t iszAttrClass;

	KeyValues *definition;

public:
	attrib_def_index_t index;
	ISchemaAttributeType *type;
	bool string_attribute;
	int description_format;
	int effect_type;
	bool hidden;
	bool stored_as_integer;
};

class CEconItemDefinition
{
public:
	CEconItemDefinition()
	{
		definition = NULL;
		index = INVALID_ITEM_DEF_INDEX;
		CLEAR_STR( name );
		used_by_classes = 0;

		for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
			item_slot_per_class[i] = -1;
		for ( int team = 0; team < TF_TEAM_COUNT; team++ )
			visual[team] = NULL;

		show_in_armory = true;
		CLEAR_STR( item_class );
		CLEAR_STR( item_type_name );
		CLEAR_STR( item_name );
		CLEAR_STR( item_description );
		item_slot = -1;
		anim_slot = -1;
		item_quality = QUALITY_VINTAGE;
		baseitem = false;
		propername = false;
		CLEAR_STR( item_logname );
		CLEAR_STR( item_iconname );
		min_ilevel = 0;
		max_ilevel = 0;
		CLEAR_STR( image_inventory );
		image_inventory_size_w = 128;
		image_inventory_size_h = 82;
		CLEAR_STR( model_player );
		CLEAR_STR( model_vision_filtered );
		CLEAR_STR( model_world );
		CLEAR_STR( equip_region );
		V_memset( &model_player_per_class, 0, sizeof( model_player_per_class ) );
		attach_to_hands = 1;
		attach_to_hands_vm_only = 0;
		CLEAR_STR( extra_wearable );
		CLEAR_STR( extra_wearable_vm );
		act_as_wearable = false;
		hide_bodygroups_deployed_only = 0;
		is_reskin = false;
		specialitem = false;
		demoknight = false;
		drop_type = DROPTYPE_NONE;
		year = 2005; // Generic value for hiding the year. (No items came out before 2006)
		is_custom_content = false;
		is_cut_content = false;
		is_multiclass_item = false;
		CLEAR_STR( holiday_restriction );
		CLEAR_STR( v_model );
		CLEAR_STR( w_model );
		can_use_old_model = 0;
	}
	~CEconItemDefinition();

	PerTeamVisuals_t *GetVisuals( int iTeamNum = TEAM_UNASSIGNED );
	char const *GetPerClassModel( int iClass = TF_CLASS_UNDEFINED );
	int GetLoadoutSlot( int iClass = TF_CLASS_UNDEFINED );
	char const *GetPlayerModel( void ) const;
	char const *GetWorldModel( void ) const;
	int GetAttachToHands( void ) const;
	const wchar_t *GenerateLocalizedFullItemName( void );
	const wchar_t *GenerateLocalizedItemNameNoQuality( void );
	void IterateAttributes( IEconAttributeIterator *iter ) const;

	KeyValues *GetStaticDefinition( void ) const { return definition; }

	char const *GetName( void ) const
	{
		Assert( name && *name );
		return name;
	}
	char const *GetVisionFilteredModel( void ) const
	{
		if ( model_vision_filtered && model_vision_filtered[0] != '\0' )
			return model_vision_filtered;

		return NULL;
	}
	char const *GetExtraWearableModel( void ) const
	{
		if ( extra_wearable && extra_wearable[0] != '\0' )
			return extra_wearable;

		return NULL;
	}
	char const *GetExtraWearableViewModel( void ) const
	{
		if ( extra_wearable_vm && extra_wearable_vm[0] != '\0' )
			return extra_wearable_vm;

		return NULL;
	}
	char const *GetLocalizationName( void ) const
	{
		Assert( item_name && *item_name );
		return item_name;
	}
	char const *GetDescription( void ) const
	{
		if ( item_description && item_description[0] != '\0' )
			return item_description;

		return NULL;
	}
	char const *GetLogName( void ) const
	{
		if (item_logname && item_logname[0] != '\0')
			return item_logname;

		return NULL;
	}
	char const *GetItemIcon( void ) const
	{
		if ( item_iconname && item_iconname[0] != '\0' )
			return item_iconname;

		return NULL;
	}
	char const *GetInventoryImage( void ) const
	{
		Assert( image_inventory && *image_inventory );
		return image_inventory;
	}
	char const *GetClassName( void ) const
	{
		Assert( item_class && *item_class );
		return item_class;
	}
	char const *GetTypeName( void ) const
	{
		if ( item_type_name && item_type_name[0] != '\0' )
			return item_type_name;

		return NULL;
	}
	char const *GetHolidayRestriction( void ) const
	{
		if ( holiday_restriction && holiday_restriction[0] != '\0' )
			return holiday_restriction;

		return NULL;
	}
	char const *GetVScriptName( void ) const
	{
		if ( item_script && item_script[0] != '\0' )
			return item_script;

		return NULL;
	}
	bool CanUseOldModel( void ) const
	{
		// Need to check if our v/w models are defined.
		if ( can_use_old_model == 1 )
			return true;
			
		return false;
	}


	bool LoadFromKV( KeyValues *pKV );
	void ParseVisuals( KeyValues *pKVData, int iIndex );
		

private:
	char const *name;
	char const *model_player;
	char const *model_vision_filtered;
	char const *model_world;
	char const *model_player_per_class[ TF_CLASS_COUNT_ALL ];
	char const *extra_wearable;
	char const *extra_wearable_vm;
	char const *item_class;
	char const *item_type_name;
	char const *item_name;
	char const *item_description;
	char const *item_logname;
	char const *item_iconname;
	char const *image_inventory;
	char const *equip_region;
	char const *holiday_restriction;
	char const *item_script;
	
	char const *v_model;
	char const *w_model;

	KeyValues *definition;

public:
	item_def_index_t index;
	CUtlVector<static_attrib_t> attributes;
	PerTeamVisuals_t *visual[ TF_TEAM_COUNT ];
	int used_by_classes;
	int item_slot_per_class[ TF_CLASS_COUNT_ALL ];
	bool show_in_armory;
	int  item_slot;
	int  anim_slot;
	int  item_quality;
	bool baseitem;
	bool propername;
	int	 min_ilevel;
	int	 max_ilevel;
	int	 image_inventory_size_w;
	int	 image_inventory_size_h;
	bool loadondemand;
	int  attach_to_hands;
	int  attach_to_hands_vm_only;
	bool act_as_wearable;
	bool act_as_weapon;
	CUtlDict< bool, unsigned short > capabilities;
	CUtlDict< bool, unsigned short > tags;
	int  hide_bodygroups_deployed_only;
	bool is_reskin;
	bool specialitem;
	bool demoknight;
	int drop_type;
	int  year;
	bool is_custom_content;
	bool is_cut_content;
	bool is_multiclass_item;
	int can_use_old_model;
};

#endif // ECON_ITEM_SCHEMA_H
