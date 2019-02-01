#ifndef CMS_HPP
#define CMS_HPP

#include <kanzi/kanzi.hpp>

using namespace kanzi;

// Application class declaration
class CMS : public Application
{
	
protected:

	// Register
	virtual void registerMetadataOverride(ObjectFactory & /*factory*/) KZ_OVERRIDE;

    // Configures application
    virtual void onConfigure(ApplicationProperties &configuration) KZ_OVERRIDE;

    // Project loading complete - execute main application initialisation
    virtual void onProjectLoaded() KZ_OVERRIDE;
	
	// Callback for application update implementation
	virtual void onUpdate(chrono::milliseconds /*deltaTime*/) KZ_OVERRIDE;

	// Callback for application shut-down handling
	virtual void onShutdown() KZ_OVERRIDE;

private:

	
private:

};

#endif // CMS_HPP
