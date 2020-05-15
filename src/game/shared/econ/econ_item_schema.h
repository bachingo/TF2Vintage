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
		name[0] = '\0'


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

	char color_name[128];
} Color_t;

class CEconAttributeDefinition
{
public:
	CEconAttributeDefinition()
	{
		index = 0xFFFF;
		CLEAR_STR( name );
		CLEAR_STR( attribute_class );
		CLEAR_STR( description_string );
		string_attribute = false;
		description_format = -1;
		hidden = false;
		effect_type = -1;
		stored_as_integer = false;
		m_iAttributeClass = NULL_STRING;
	}

	unsigned short index;
	ISchemaAttributeType *type;
	char name[128];
	char attribute_class[128];
	char description_string[128];
	bool string_attribute;
	int description_format;
	int effect_type;
	bool hidden;
	bool stored_as_integer;
	string_t m_iAttributeClass;
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

	int skin_red;
	int skin_blu;
	bool selectable;
	char name[128];
	char model_player[128];
	char image_inventory[128];
	CUtlDict< const char*, unsigned short > model_player_per_class;
} ItemStyle_t;

typedef struct EconPerTeamVisuals
{
	EconPerTeamVisuals()
	{
		animation_replacement.SetLessFunc( [ ] ( const int &lhs, const int &rhs ) -> bool { return lhs < rhs; } );
		V_memset( aWeaponSounds, 0, sizeof( aWeaponSounds ) );
		CLEAR_STR( custom_particlesystem );
		CLEAR_STR( muzzle_flash );
		CLEAR_STR( tracer_effect );
		skin = -1;
		use_per_class_bodygroups = 0;
	}

	CUtlDict< bool, unsigned short > player_bodygroups;
	CUtlMap< int, int > animation_replacement;
	CUtlDict< const char*, unsigned short > playback_activity;
	CUtlDict< const char*, unsigned short > misc_info;
	CUtlVector< AttachedModel_t > attached_models;
	char aWeaponSounds[NUM_SHOOT_SOUND_TYPES][MAX_WEAPON_STRING];
	char custom_particlesystem[128];
	char muzzle_flash[128];
	char tracer_effect[128];
	CUtlDict< ItemStyle_t*, unsigned short > styles;
	int skin;
	int use_per_class_bodygroups;
} PerTeamVisuals_t;

class CEconItemDefinition
{
public:
	CEconItemDefinition()
	{
		m_pDefinition = NULL;
		index = 0xFFFFFFFF;
		CLEAR_STR( name );
		used_by_classes = 0;

		for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
			item_slot_per_class[i] = -1;

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
		Q_memset( &model_player_per_class, 0, sizeof( model_player_per_class ) );
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
	int GetLoadoutSlot( int iClass = TF_CLASS_UNDEFINED );
	const wchar_t *GenerateLocalizedFullItemName( void );
	const wchar_t *GenerateLocalizedItemNameNoQuality( void );
	void IterateAttributes( IEconAttributeIterator *iter );

public:
	KeyValues *m_pDefinition;
	unsigned int index;
	char name[128];
	CUtlDict< bool, unsigned short > capabilities;
	CUtlDict< bool, unsigned short > tags;
	int used_by_classes;
	int item_slot_per_class[TF_CLASS_COUNT_ALL];
	bool show_in_armory;
	char item_class[128];
	char item_type_name[128];
	char item_name[128];
	char item_description[128];
	int  item_slot;
	int  anim_slot;
	int  item_quality;
	bool baseitem;
	bool propername;
	char item_logname[128];
	char item_iconname[128];
	int	 min_ilevel;
	int	 max_ilevel;
	char image_inventory[128];
	int	 image_inventory_size_w;
	int	 image_inventory_size_h;
	char model_player[128];
	char model_vision_filtered[128];
	char model_world[128];
	char equip_region[128];
	char model_player_per_class[TF_CLASS_COUNT_ALL][128];
	char extra_wearable[128];
	bool loadondemand;
	int  attach_to_hands;
	int  attach_to_hands_vm_only;
	bool act_as_wearable;
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
	
	CUtlVector<static_attrib_t> attributes;
	PerTeamVisuals_t visual[TF_TEAM_COUNT];
};


class IEconAttributeIterator
{
public:
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, unsigned int ) = 0;
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, float ) = 0;
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, CAttribute_String const & ) = 0;
	virtual bool OnIterateAttributeValue( CEconAttributeDefinition const *, uint64 const & ) = 0;
};

#endif // ECON_ITEM_SCHEMA_H
