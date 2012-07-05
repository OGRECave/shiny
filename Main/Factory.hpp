#ifndef SH_FACTORY_H
#define SH_FACTORY_H

#include <map>
#include <string>

#include "MaterialInstance.hpp"
#include "ShaderSet.hpp"
#include "Language.hpp"

namespace sh
{
	class Platform;

	typedef std::map<std::string, MaterialInstance> MaterialMap;
	typedef std::map<std::string, ShaderSet> ShaderSetMap;
	typedef std::map<std::string, std::string> SettingsMap;
	typedef std::map<std::string, PropertySetGet> ConfigurationMap;

	typedef std::map<std::string, std::string> TextureAliasMap;

	/**
	 * @brief
	 * The main interface class
	 */
	class Factory
	{
	public:
		Factory(Platform* platform);
		///< @note Ownership of \a platform is transferred to this class, so you don't have to delete it.

		~Factory();

		/**
		 * Create a MaterialInstance, optionally copying all properties from \a parentInstance
		 * @param name name of the new instance
		 * @param name of the parent (optional)
		 * @return newly created instance
		 */
		MaterialInstance* createMaterialInstance (const std::string& name, const std::string& parentInstance = "");

		/// @note It is safe to call this if the instance does not exist
		void destroyMaterialInstance (const std::string& name);

		/// Use this to enable or disable shaders on-the-fly
		void setShadersEnabled (bool enabled);

		/// Use this to manage user settings. \n
		/// Global settings can be retrieved in shaders through a macro. \n
		/// When a global setting is changed, the shaders that depend on them are recompiled automatically.
		void setGlobalSetting (const std::string& name, const std::string& value);

		/// Adjusts the given shared parameter. \n
		/// Internally, this will change all uniform parameters of this name marked with the macro \@shSharedParameter \n
		/// @param name of the shared parameter
		/// @param value of the parameter, use sh::makeProperty to construct this value
		void setSharedParameter (const std::string& name, PropertyValuePtr value);

		Language getCurrentLanguage ();

		/// Switch between different shader languages (cg, glsl, hlsl)
		void setCurrentLanguage (Language lang);

		/// Get a MaterialInstance by name
		MaterialInstance* getInstance (const std::string& name);

		/// Register a \a Configuration, which can then be used by switching the active material scheme
		void registerConfiguration (const std::string& name, PropertySetGet configuration);

		/// Set an alias name for a texture, the real name can then be retrieved with the "texture_alias"
		/// property in a texture unit - this is useful if you don't know the name of your texture beforehand. \n
		/// Example: \n
		///  - In the material definition: texture_alias ReflectionMap \n
		///  - At runtime: factory->setTextureAlias("ReflectionMap", "rtt_654654"); \n
		/// You can call factory->setTextureAlias as many times as you want, and if the material was already created, its texture will be updated!
		/// @note If the material with a texture alias is requested for rendering before this method was called, will result in a crash,
		/// so make sure to do it in time.
		void setTextureAlias (const std::string& alias, const std::string& realName);

		/// Retrieve the real texture name for a texture alias (the real name is set by the user)
		std::string retrieveTextureAlias (const std::string& name);

		void notifyFrameEntered ();

		static Factory& getInstance();
		///< Return instance of this class.

	private:

		MaterialInstance* requestMaterial (const std::string& name, const std::string& configuration);
		ShaderSet* getShaderSet (const std::string& name);
		PropertySetGet* getConfiguration (const std::string& name);
		Platform* getPlatform ();

		friend class Platform;
		friend class MaterialInstance;
		friend class ShaderInstance;

	private:
		static Factory* sThis;

		bool mShadersEnabled;

		MaterialMap mMaterials;
		ShaderSetMap mShaderSets;
		SettingsMap mGlobalSettings;
		ConfigurationMap mConfigurations;

		TextureAliasMap mTextureAliases;

		std::string mCachePath;

		Language mCurrentLanguage;

		Platform* mPlatform;

		MaterialInstance* findInstance (const std::string& name);
		MaterialInstance* searchInstance (const std::string& name);
	};
};

#endif
