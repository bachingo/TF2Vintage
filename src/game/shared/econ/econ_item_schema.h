#ifndef ECON_ITEM_SCHEMA_H
#define ECON_ITEM_SCHEMA_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"
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
	DROPTYPE_NONE,
	DROPTYPE_DROP,
	DROPTYPE_BREAK,
};

extern const char *g_szQualityColorStrings[];
extern const char *g_szQualityLocalizationStrings[];

#define CALL_ATTRIB_HOOK_INT(value, name, ...) \
		value = CAttributeManager::AttribHookValue<int>(value, #name, this, __VA_ARGS__)

#define CALL_ATTRIB_HOOK_FLOAT(value, name, ...) \
		value = CAttributeManager::AttribHookValue<float>(value, #name, this, __VA_ARGS__)

#define CALL_ATTRIB_HOOK_STRING(value, name, ...) \
		value = CAttributeManager::AttribHookValue<string_t>(value, #name, this, __VA_ARGS__)

#define CALL_ATTRIB_HOOK_INT_ON_OTHER(ent, value, name, ...) \
		value = CAttributeManager::AttribHookValue<int>(value, #name, ent, __VA_ARGS__)

#define CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(ent, value, name, ...) \
		value = CAttributeManager::AttribHookValue<float>(value, #name, ent, __VA_ARGS__)

#define CALL_ATTRIB_HOOK_STRING_ON_OTHER(ent, value, name, ...) \
		value = CAttributeManager::AttribHookValue<string_t>(value, #name, ent, __VA_ARGS__)

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
		m_bInitialized = true;

		if ( m_pString == &_default_value_ )
			m_pString = new CUtlConstString;
	}

	void CopyFrom( CAttribute_String const &src )
	{
		Clear();

		*this = src;
	}

	void Assign( char const *src )
	{
		Initialize();

		*m_pString = src;
	}

	void Clear( void );

	CAttribute_String &operator=( CAttribute_String const &src );
	CAttribute_String &operator=( char const *src );

#if defined GAME_DLL
	CAttribute_String &operator=( string_t const &src )
	{
		return this->operator=( STRING( src ) );
	}
#endif

	// Inner string access
	const char *Get( void ) const { return m_pString->Get(); }
	CUtlConstString *GetForModify( void ) { return m_pString; }

	operator char const *( ) { return Get(); }
	operator char const *( ) const { return Get(); }

#if defined GAME_DLL
	operator string_t ( ) { return MAKE_STRING( Get() ); }
	operator string_t ( ) const { return MAKE_STRING( Get() ); }
#endif

protected:
	static CUtlConstString _default_value_;

	CUtlConstString *m_pString;
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

class CEconAttributeDefinition
{
public:
	CEconAttributeDefinition()
	{
		definition = NULL;
		index = 0xFFFF;
		name[0] = '\0';
		attribute_class[0] = '\0';
		description_string[0] = '\0';
		string_attribute = false;
		description_format = -1;
		hidden = false;
		effect_type = -1;
		stored_as_integer = false;
		m_iAttributeClass = NULL_STRING;
	}
	~CEconAttributeDefinition()
	{
		definition->deleteThis();
	}

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

private:
	char name[128];
	char attribute_class[64];
	char description_string[64];

	KeyValues *definition;

public:
	unsigned short index;
	ISchemaAttributeType *type;
	bool string_attribute;
	int description_format;
	int effect_type;
	bool hidden;
	bool stored_as_integer;

	mutable string_t m_iAttributeClass;

	friend class CEconItemSchema;
	friend class CEconSchemaParser;
};

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
	unsigned iVal;
	float flVal;
	uint64 *lVal;
	CAttribute_String *sVal;
} attrib_data_union_t;
static_assert( sizeof( attrib_data_union_t ) == 4, "If the size changes you've done something wrong!" );

typedef struct
{
	CEconAttributeDefinition const *GetStaticData( void ) const
	{
		return GetItemSchema()->GetAttributeDefinition( iAttribIndex );
	}
	bool BInitFromKV_SingleLine( KeyValues *const kv );
	bool BInitFromKV_MultiLine( KeyValues *const kv );

	unsigned short iAttribIndex;
	attrib_data_union_t value;
} static_attrib_t;


// Client specific.
#ifdef CLIENT_DLL
	EXTERN_RECV_TABLE( DT_EconItemAttribute );
// Server specific.
#else
	EXTERN_SEND_TABLE( DT_EconItemAttribute );
#endif

class CEconItemAttribute
{
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_CLASS_NOBASE( CEconItemAttribute );

	CEconAttributeDefinition *GetStaticData( void );

	CEconItemAttribute()
	{
		Init( -1, 0.0f );
	}
	CEconItemAttribute( int iIndex, float flValue )
	{
		Init( iIndex, flValue );
	}
	CEconItemAttribute( int iIndex, float flValue, const char *pszAttributeClass )
	{
		Init( iIndex, flValue, pszAttributeClass );
	}
	CEconItemAttribute( int iIndex, const char *pszValue, const char *pszAttributeClass )
	{
		Init( iIndex, pszValue, pszAttributeClass );
	}
	CEconItemAttribute( CEconItemAttribute const &src );

	void Init( int iIndex, float flValue, const char *pszAttributeClass = NULL );
	void Init( int iIndex, const char *iszValue, const char *pszAttributeClass = NULL );

	CEconItemAttribute &operator=( CEconItemAttribute const &src );

public:
	CNetworkVar( uint16, m_iAttributeDefinitionIndex );
	CNetworkVar( float, m_flValue );
	string_t m_iAttributeClass;
};

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

	friend class CEconItemSchema;
	friend class CEconSchemaParser;
} ItemStyle_t;

#define MAX_CUSTOM_WEAPON_SOUNDS   10
typedef struct EconPerTeamVisuals
{
	EconPerTeamVisuals()
	{
		animation_replacement.SetLessFunc( [ ] ( const int &lhs, const int &rhs ) -> bool { return lhs < rhs; } );
		V_memset( &aCustomWeaponSounds, 0, sizeof( aCustomWeaponSounds ) );
		V_memset( &aWeaponSounds, 0, sizeof( aWeaponSounds ) );
		CLEAR_STR( custom_particlesystem );
		CLEAR_STR( muzzle_flash );
		CLEAR_STR( tracer_effect );
		CLEAR_STR( material_override );
		skin = -1;
		use_per_class_bodygroups = 0;
	}
	~EconPerTeamVisuals()
	{
		playback_activity.PurgeAndDeleteElements();
		misc_info.PurgeAndDeleteElements();
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
		Assert( sound >= 0 && sound <= MAX_CUSTOM_WEAPON_SOUNDS );
		if ( aWeaponSounds[sound] && aWeaponSounds[sound][0] != '\0' )
			return aWeaponSounds[sound];

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

private:
	char const *aCustomWeaponSounds[ MAX_CUSTOM_WEAPON_SOUNDS ];
	char const *aWeaponSounds[ NUM_SHOOT_SOUND_TYPES ];
	char const *custom_particlesystem;
	char const *muzzle_flash;
	char const *tracer_effect;
	char const *material_override;

public:
	CUtlDict< bool, unsigned short > player_bodygroups;
	CUtlMap< int, int > animation_replacement;
	int skin;
	int use_per_class_bodygroups;
	CUtlDict< const char*, unsigned short > playback_activity;
	CUtlDict< const char*, unsigned short > misc_info;
	CUtlVector< AttachedModel_t > attached_models;
	CUtlDict< ItemStyle_t*, unsigned short > styles;

	friend class CEconItemSchema;
	friend class CEconSchemaParser;
} PerTeamVisuals_t;

class CEconItemDefinition
{
public:
	CEconItemDefinition()
	{
		definition = NULL;
		index = 0xFFFFFFFF;
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
	}
	~CEconItemDefinition();

	PerTeamVisuals_t *GetVisuals( int iTeamNum = TEAM_UNASSIGNED );
	char const *GetPerClassModel( int iClass = TF_CLASS_UNDEFINED );
	int GetLoadoutSlot( int iClass = TF_CLASS_UNDEFINED );
	const wchar_t *GenerateLocalizedFullItemName( void );
	const wchar_t *GenerateLocalizedItemNameNoQuality( void );
	void IterateAttributes( IEconAttributeIterator *iter );

	char const *GetName( void ) const
	{
		Assert( name && *name );
		return name;
	}
	char const *GetPlayerModel( void ) const
	{
		if ( model_player && model_player[0] != '\0' )
			return model_player;

		return NULL;
	}
	char const *GetWorldModel( void ) const
	{
		if ( model_world && model_world[0] != '\0' )
			return model_world;

		return NULL;
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

private:
	char const *name;
	char const *model_player;
	char const *model_vision_filtered;
	char const *model_world;
	char const *model_player_per_class[ TF_CLASS_COUNT_ALL ];
	char const *extra_wearable;
	char const *item_class;
	char const *item_type_name;
	char const *item_name;
	char const *item_description;
	char const *item_logname;
	char const *item_iconname;
	char const *image_inventory;
	char const *equip_region;
	char const *holiday_restriction;

	KeyValues *definition;

public:
	unsigned int index;
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
	char model_player[128];
	bool loadondemand;
	int  attach_to_hands;
	int  attach_to_hands_vm_only;
	bool act_as_wearable;
	CUtlDict< bool, unsigned short > capabilities;
	int  hide_bodygroups_deployed_only;
	bool is_reskin;
	bool specialitem;
	bool demoknight;
	char holiday_restriction[128];
	int drop_type;
	int  year;
	bool is_custom_content;
	bool is_cut_content;
	bool is_multiclass_item;


class IEconAttributeIterator
{
public:
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, unsigned int ) = 0;
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, float ) = 0;
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, CAttribute_String const & ) = 0;
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, uint64 const & ) = 0;
	friend class CEconItemSchema;
	friend class CEconSchemaParser;
};

#endif // ECON_ITEM_SCHEMA_H
