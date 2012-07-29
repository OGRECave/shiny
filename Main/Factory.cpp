#include "Factory.hpp"

#include <stdexcept>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "Platform.hpp"
#include "ScriptLoader.hpp"
#include "ShaderSet.hpp"
#include "MaterialInstanceTextureUnit.hpp"

namespace sh
{
	Factory* Factory::sThis = 0;

	Factory& Factory::getInstance()
	{
		assert (sThis);
		return *sThis;
	}

	Factory* Factory::getInstancePtr()
	{
		return sThis;
	}

	Factory::Factory (Platform* platform)
		: mPlatform(platform)
		, mShadersEnabled(true)
		, mCurrentLanguage(Language_None)
		, mListener(NULL)
		, mCurrentConfiguration(NULL)
		, mCurrentLodConfiguration(NULL)
	{
		assert (!sThis);
		sThis = this;

		mPlatform->setFactory(this);

		if (mPlatform->supportsShaderSerialization ())
		{
			std::string file = mPlatform->getCacheFolder () + "/shShaderCache.txt";
			if (boost::filesystem::exists(file))
			{
				mPlatform->deserializeShaders (file);
			}
		}
	}

	void Factory::loadAllFiles()
	{
		assert(mCurrentLanguage != Language_None);

		// load configurations
		{
			ScriptLoader shaderSetLoader(".configuration");
			ScriptLoader::loadAllFiles (&shaderSetLoader, mPlatform->getBasePath());
			std::map <std::string, ScriptNode*> nodes = shaderSetLoader.getAllConfigScripts();
			for (std::map <std::string, ScriptNode*>::const_iterator it = nodes.begin();
				it != nodes.end(); ++it)
			{
				if (!(it->second->getName() == "configuration"))
				{
					std::cerr << "sh::Factory: Warning: Unsupported root node type \"" << it->second->getName() << "\" for file type .configuration" << std::endl;
					break;
				}

				PropertySetGet newConfiguration;
				newConfiguration.setParent(&mGlobalSettings);

				std::vector<ScriptNode*> props = it->second->getChildren();
				for (std::vector<ScriptNode*>::const_iterator propIt = props.begin(); propIt != props.end(); ++propIt)
				{
					std::string name = (*propIt)->getName();
					std::string val = (*propIt)->getValue();

					newConfiguration.setProperty (name, makeProperty(val));
				}

				mConfigurations[it->first] = newConfiguration;
			}
		}

		// load lod configurations
		{
			ScriptLoader lodLoader(".lod");
			ScriptLoader::loadAllFiles (&lodLoader, mPlatform->getBasePath());
			std::map <std::string, ScriptNode*> nodes = lodLoader.getAllConfigScripts();
			for (std::map <std::string, ScriptNode*>::const_iterator it = nodes.begin();
				it != nodes.end(); ++it)
			{
				if (!(it->second->getName() == "lod_configuration"))
				{
					std::cerr << "sh::Factory: Warning: Unsupported root node type \"" << it->second->getName() << "\" for file type .lod" << std::endl;
					break;
				}

				if (it->first == "0")
				{
					throw std::runtime_error("lod level 0 (max lod) can't have a configuration");
				}

				PropertySetGet newLod;

				std::vector<ScriptNode*> props = it->second->getChildren();
				for (std::vector<ScriptNode*>::const_iterator propIt = props.begin(); propIt != props.end(); ++propIt)
				{
					std::string name = (*propIt)->getName();
					std::string val = (*propIt)->getValue();

					newLod.setProperty (name, makeProperty(val));
				}

				mLodConfigurations[boost::lexical_cast<int>(it->first)] = newLod;
			}
		}

		// load shader sets
		{
			ScriptLoader shaderSetLoader(".shaderset");
			ScriptLoader::loadAllFiles (&shaderSetLoader, mPlatform->getBasePath());
			std::map <std::string, ScriptNode*> nodes = shaderSetLoader.getAllConfigScripts();
			for (std::map <std::string, ScriptNode*>::const_iterator it = nodes.begin();
				it != nodes.end(); ++it)
			{
				if (!(it->second->getName() == "shader_set"))
				{
					std::cerr << "sh::Factory: Warning: Unsupported root node type \"" << it->second->getName() << "\" for file type .shaderset" << std::endl;
					break;
				}

				if (!it->second->findChild("profiles_cg"))
					throw std::runtime_error ("missing \"profiles_cg\" field for \"" + it->first + "\"");
				if (!it->second->findChild("profiles_hlsl"))
					throw std::runtime_error ("missing \"profiles_hlsl\" field for \"" + it->first + "\"");
				if (!it->second->findChild("source"))
					throw std::runtime_error ("missing \"source\" field for \"" + it->first + "\"");
				if (!it->second->findChild("type"))
					throw std::runtime_error ("missing \"type\" field for \"" + it->first + "\"");

				std::vector<std::string> profiles_cg;
				boost::split (profiles_cg, it->second->findChild("profiles_cg")->getValue(), boost::is_any_of(" "));
				std::string cg_profile;
				for (std::vector<std::string>::iterator it2 = profiles_cg.begin(); it2 != profiles_cg.end(); ++it2)
				{
					if (mPlatform->isProfileSupported(*it2))
					{
						cg_profile = *it2;
						break;
					}
				}

				std::vector<std::string> profiles_hlsl;
				boost::split (profiles_hlsl, it->second->findChild("profiles_hlsl")->getValue(), boost::is_any_of(" "));
				std::string hlsl_profile;
				for (std::vector<std::string>::iterator it2 = profiles_hlsl.begin(); it2 != profiles_hlsl.end(); ++it2)
				{
					if (mPlatform->isProfileSupported(*it2))
					{
						hlsl_profile = *it2;
						break;
					}
				}

				ShaderSet newSet (it->second->findChild("type")->getValue(), cg_profile, hlsl_profile,
								  mPlatform->getBasePath() + "/" + it->second->findChild("source")->getValue(),
								  mPlatform->getBasePath(),
								  it->first,
								  &mGlobalSettings);

				mShaderSets.insert(std::make_pair(it->first, newSet));
			}
		}

		// load materials
		{
			ScriptLoader materialLoader(".mat");
			ScriptLoader::loadAllFiles (&materialLoader, mPlatform->getBasePath());

			std::map <std::string, ScriptNode*> nodes = materialLoader.getAllConfigScripts();
			for (std::map <std::string, ScriptNode*>::const_iterator it = nodes.begin();
				it != nodes.end(); ++it)
			{
				if (!(it->second->getName() == "material"))
				{
					std::cerr << "sh::Factory: Warning: Unsupported root node type \"" << it->second->getName() << "\" for file type .mat" << std::endl;
					break;
				}

				MaterialInstance newInstance(it->first, this);
				newInstance.create(mPlatform);
				if (!mShadersEnabled)
					newInstance.setShadersEnabled (false);

				std::vector<ScriptNode*> props = it->second->getChildren();
				for (std::vector<ScriptNode*>::const_iterator propIt = props.begin(); propIt != props.end(); ++propIt)
				{
					std::string name = (*propIt)->getName();

					std::string val = (*propIt)->getValue();

					if (name == "pass")
					{
						MaterialInstancePass* newPass = newInstance.createPass();
						std::vector<ScriptNode*> props2 = (*propIt)->getChildren();
						for (std::vector<ScriptNode*>::const_iterator propIt2 = props2.begin(); propIt2 != props2.end(); ++propIt2)
						{
							std::string name2 = (*propIt2)->getName();
							std::string val2 = (*propIt2)->getValue();

							if (name2 == "shader_properties")
							{
								std::vector<ScriptNode*> shaderProps = (*propIt2)->getChildren();
								for (std::vector<ScriptNode*>::const_iterator shaderPropIt = shaderProps.begin(); shaderPropIt != shaderProps.end(); ++shaderPropIt)
								{
									std::string val = (*shaderPropIt)->getValue();
									newPass->mShaderProperties.setProperty((*shaderPropIt)->getName(), makeProperty(val));
								}
							}
							else if (name2 == "texture_unit")
							{
								MaterialInstanceTextureUnit* newTex = newPass->createTextureUnit(val2);
								std::vector<ScriptNode*> texProps = (*propIt2)->getChildren();
								for (std::vector<ScriptNode*>::const_iterator texPropIt = texProps.begin(); texPropIt != texProps.end(); ++texPropIt)
								{
									std::string val = (*texPropIt)->getValue();
									newTex->setProperty((*texPropIt)->getName(), makeProperty(val));
								}
							}
							else
								newPass->setProperty((*propIt2)->getName(), makeProperty(val2));
						}
					}
					else if (name == "parent")
						newInstance.setParentInstance(val);
					else
						newInstance.setProperty((*propIt)->getName(), makeProperty(val));
				}

				if (newInstance.hasProperty("create_configuration"))
				{
					std::string config = retrieveValue<StringValue>(newInstance.getProperty("create_configuration"), NULL).get();
					newInstance.createForConfiguration (config, 0);
				}

				mMaterials.insert (std::make_pair(it->first, newInstance));
			}

			// now that all materials are loaded, replace the parent names with the actual pointers to parent
			for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
			{
				std::string parent = it->second.getParentInstance();
				if (parent != "")
				{
					if (mMaterials.find (it->second.getParentInstance()) == mMaterials.end())
						throw std::runtime_error ("Unable to find parent for material instance \"" + it->first + "\"");
					it->second.setParent(&mMaterials.find(parent)->second);
				}
			}
		}
	}

	Factory::~Factory ()
	{
		if (mPlatform->supportsShaderSerialization ())
		{
			std::string file = mPlatform->getCacheFolder () + "/shShaderCache.txt";
			mPlatform->serializeShaders (file);
		}

		delete mPlatform;
		sThis = 0;
	}

	MaterialInstance* Factory::searchInstance (const std::string& name)
	{
		if (mMaterials.find(name) != mMaterials.end())
				return &mMaterials.find(name)->second;

		return NULL;
	}

	MaterialInstance* Factory::findInstance (const std::string& name)
	{
		assert (mMaterials.find(name) != mMaterials.end());
		return &mMaterials.find(name)->second;
	}

	MaterialInstance* Factory::requestMaterial (const std::string& name, const std::string& configuration, unsigned short lodIndex)
	{
		MaterialInstance* m = searchInstance (name);

		if (m)
		{
			// make sure all lod techniques below (higher lod) exist
			int i = lodIndex;
			while (i>0)
			{
				--i;
				m->createForConfiguration (configuration, i);

				if (mListener)
					mListener->materialCreated (m, configuration, i);
			}

			m->createForConfiguration (configuration, lodIndex);
			if (mListener)
				mListener->materialCreated (m, configuration, lodIndex);
		}
		return m;
	}

	MaterialInstance* Factory::createMaterialInstance (const std::string& name, const std::string& parentInstance)
	{
		if (parentInstance != "" && mMaterials.find(parentInstance) == mMaterials.end())
			throw std::runtime_error ("trying to clone material that does not exist");

		MaterialInstance newInstance(name, this);

		if (!mShadersEnabled)
			newInstance.setShadersEnabled(false);

		if (parentInstance != "")
			newInstance.setParent (&mMaterials.find(parentInstance)->second);

		newInstance.create(mPlatform);

		mMaterials.insert (std::make_pair(name, newInstance));

		return &mMaterials.find(name)->second;
	}

	void Factory::destroyMaterialInstance (const std::string& name)
	{
		if (mMaterials.find(name) != mMaterials.end())
			mMaterials.erase(name);
	}

	void Factory::setShadersEnabled (bool enabled)
	{
		mShadersEnabled = enabled;
		for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
		{
			it->second.setShadersEnabled(enabled);
		}
	}

	void Factory::setGlobalSetting (const std::string& name, const std::string& value)
	{
		bool changed = true;
		if (mGlobalSettings.hasProperty(name))
			changed = (retrieveValue<StringValue>(mGlobalSettings.getProperty(name), NULL).get() != value);

		mGlobalSettings.setProperty (name, makeProperty<StringValue>(new StringValue(value)));

		if (changed)
		{
			for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
			{
				it->second.destroyAll();
			}
		}
	}

	void Factory::setSharedParameter (const std::string& name, PropertyValuePtr value)
	{
		mPlatform->setSharedParameter(name, value);
	}

	ShaderSet* Factory::getShaderSet (const std::string& name)
	{
		return &mShaderSets.find(name)->second;
	}

	Platform* Factory::getPlatform ()
	{
		return mPlatform;
	}

	Language Factory::getCurrentLanguage ()
	{
		return mCurrentLanguage;
	}

	void Factory::setCurrentLanguage (Language lang)
	{
		bool changed = (mCurrentLanguage != lang);
		mCurrentLanguage = lang;

		if (changed)
		{
			for (MaterialMap::iterator it = mMaterials.begin(); it != mMaterials.end(); ++it)
			{
				it->second.destroyAll();
			}
		}
	}

	MaterialInstance* Factory::getMaterialInstance (const std::string& name)
	{
		return findInstance(name);
	}

	void Factory::setTextureAlias (const std::string& alias, const std::string& realName)
	{
		mTextureAliases[alias] = realName;

		// update the already existing texture units
		for (std::map<TextureUnitState*, std::string>::iterator it = mTextureAliasInstances.begin(); it != mTextureAliasInstances.end(); ++it)
		{
			if (it->second == alias)
			{
				it->first->setTextureName(realName);
			}
		}
	}

	std::string Factory::retrieveTextureAlias (const std::string& name)
	{
		if (mTextureAliases.find(name) != mTextureAliases.end())
			return mTextureAliases[name];
		else
			return "";
	}

	PropertySetGet* Factory::getConfiguration (const std::string& name)
	{
		return &mConfigurations[name];
	}

	void Factory::registerConfiguration (const std::string& name, PropertySetGet configuration)
	{
		mConfigurations[name] = configuration;
		mConfigurations[name].setParent (&mGlobalSettings);
	}

	void Factory::registerLodConfiguration (int index, PropertySetGet configuration)
	{
		mLodConfigurations[index] = configuration;
	}

	void Factory::setMaterialListener (MaterialListener* listener)
	{
		mListener = listener;
	}

	void Factory::addTextureAliasInstance (const std::string& name, TextureUnitState* t)
	{
		mTextureAliasInstances[t] = name;
	}

	void Factory::removeTextureAliasInstances (TextureUnitState* t)
	{
		mTextureAliasInstances.erase(t);
	}

	void Factory::setActiveConfiguration (const std::string& configuration)
	{
		if (configuration == "Default")
			mCurrentConfiguration = 0;
		else
			mCurrentConfiguration = &mConfigurations[configuration];
	}

	void Factory::setActiveLodLevel (int level)
	{
		if (level == 0)
			mCurrentLodConfiguration = 0;
		else
		{
			mCurrentLodConfiguration = &mLodConfigurations[level];
		}
	}

	PropertySetGet* Factory::getCurrentGlobalSettings()
	{
		PropertySetGet* p = &mGlobalSettings;

		// current global settings are affected by active configuration & active lod configuration

		if (mCurrentConfiguration)
		{
			p = mCurrentConfiguration;
		}

		if (mCurrentLodConfiguration)
		{
			mCurrentLodConfiguration->setParent(p);
			p = mCurrentLodConfiguration;
		}

		return p;
	}
}
