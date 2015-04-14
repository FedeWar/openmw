#include "renderingmanager.hpp"

#include <stdexcept>

#include <osg/io_utils>
#include <osg/Light>
#include <osg/LightModel>
#include <osg/Group>

#include <osgViewer/Viewer>

#include <components/sceneutil/util.hpp>

#include <components/sceneutil/lightmanager.hpp>

#include <components/sceneutil/statesetcontroller.hpp>

#include <components/esm/loadcell.hpp>

#include "sky.hpp"

namespace MWRender
{

    class StateUpdater : public SceneUtil::StateSetController
    {
    public:
        virtual void setDefaults(osg::StateSet *stateset)
        {
            osg::LightModel* lightModel = new osg::LightModel;
            stateset->setAttribute(lightModel, osg::StateAttribute::ON);
        }

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor*)
        {
            osg::LightModel* lightModel = static_cast<osg::LightModel*>(stateset->getAttribute(osg::StateAttribute::LIGHTMODEL));
            lightModel->setAmbientIntensity(mAmbientColor);
        }

        void setAmbientColor(osg::Vec4f col)
        {
            mAmbientColor = col;
        }

    private:
        osg::Vec4f mAmbientColor;
    };

    RenderingManager::RenderingManager(osgViewer::Viewer &viewer, osg::ref_ptr<osg::Group> rootNode, Resource::ResourceSystem* resourceSystem)
        : mViewer(viewer)
        , mRootNode(rootNode)
        , mResourceSystem(resourceSystem)
    {
        osg::ref_ptr<SceneUtil::LightManager> lightRoot = new SceneUtil::LightManager;
        lightRoot->setStartLight(1);

        mRootNode->addChild(lightRoot);

        mObjects.reset(new Objects(mResourceSystem, lightRoot));

        mSky.reset(new SkyManager(mRootNode, resourceSystem->getSceneManager()));

        mViewer.setLightingMode(osgViewer::View::NO_LIGHT);

        osg::ref_ptr<osg::LightSource> source = new osg::LightSource;
        mSunLight = new osg::Light;
        source->setLight(mSunLight);
        mSunLight->setDiffuse(osg::Vec4f(0,0,0,1));
        mSunLight->setAmbient(osg::Vec4f(0,0,0,1));
        mSunLight->setConstantAttenuation(1.f);
        lightRoot->addChild(source);

        mRootNode->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
        mRootNode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::ON);
        mRootNode->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);

        source->setStateSetModes(*mRootNode->getOrCreateStateSet(), osg::StateAttribute::ON);

        mStateUpdater = new StateUpdater;
        mRootNode->addUpdateCallback(mStateUpdater);

        // for consistent benchmarks against the ogre branch. remove later
        osg::CullStack::CullingMode cullingMode = viewer.getCamera()->getCullingMode();
        cullingMode &= ~(osg::CullStack::SMALL_FEATURE_CULLING);
        viewer.getCamera()->setCullingMode( cullingMode );

        double fovy, aspect, zNear, zFar;
        mViewer.getCamera()->getProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar);
        fovy = 55.f;
        mViewer.getCamera()->setProjectionMatrixAsPerspective(fovy, aspect, zNear, zFar);
    }

    RenderingManager::~RenderingManager()
    {
    }

    MWRender::Objects& RenderingManager::getObjects()
    {
        return *mObjects.get();
    }

    MWRender::Actors& RenderingManager::getActors()
    {
        throw std::runtime_error("unimplemented");
    }

    Resource::ResourceSystem* RenderingManager::getResourceSystem()
    {
        return mResourceSystem;
    }

    void RenderingManager::setAmbientColour(const osg::Vec4f &colour)
    {
        mStateUpdater->setAmbientColor(colour);
    }

    void RenderingManager::configureAmbient(const ESM::Cell *cell)
    {
        setAmbientColour(SceneUtil::colourFromRGB(cell->mAmbi.mAmbient));

        mSunLight->setDiffuse(SceneUtil::colourFromRGB(cell->mAmbi.mSunlight));
        mSunLight->setDirection(osg::Vec3f(1.f,-1.f,-1.f));
    }

    void RenderingManager::setSunColour(const osg::Vec4f &colour)
    {
        mSunLight->setDiffuse(colour);
    }

    void RenderingManager::setSunDirection(const osg::Vec3f &direction)
    {
        mSunLight->setDirection(direction*-1);

        mSky->setSunDirection(direction*-1);
    }

    osg::Vec3f RenderingManager::getEyePos()
    {
        osg::Vec3d eye = mViewer.getCameraManipulator()->getMatrix().getTrans();
        return eye;
    }

    void RenderingManager::removeCell(const MWWorld::CellStore *store)
    {
        mObjects->removeCell(store);
    }

    void RenderingManager::setSkyEnabled(bool enabled)
    {
        mSky->setEnabled(enabled);
    }

    void RenderingManager::configureFog(float fogDepth, const osg::Vec4f &colour)
    {
        mViewer.getCamera()->setClearColor(colour);
    }

    SkyManager* RenderingManager::getSkyManager()
    {
        return mSky.get();
    }

}
