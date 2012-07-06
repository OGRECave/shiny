#include "OgreTextureUnitState.hpp"

#include "OgrePass.hpp"
#include "OgrePlatform.hpp"
#include "OgreMaterialSerializer.hpp"

namespace sh
{
	OgreTextureUnitState::OgreTextureUnitState (OgrePass* parent)
		: TextureUnitState()
	{
		mTextureUnitState = parent->getOgrePass()->createTextureUnitState("");
	}

	bool OgreTextureUnitState::setPropertyOverride (const std::string &name, PropertyValuePtr& value, PropertySetGet* context)
	{
		OgreMaterialSerializer& s = OgrePlatform::getSerializer();

		if (name == "texture_alias")
		{
			// texture alias in this library refers to something else than in ogre
			// delegate up
			return TextureUnitState::setPropertyOverride (name, value, context);
		}

		return s.setTextureUnitProperty (name, retrieveValue<StringValue>(value, context).get(), mTextureUnitState);
	}

	void OgreTextureUnitState::setTextureName (const std::string& textureName)
	{
		mTextureUnitState->setTextureName(textureName);
	}
}
