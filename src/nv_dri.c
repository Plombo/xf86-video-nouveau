#include "nv_include.h"

#define _XF86DRI_SERVER_
#include "GL/glxint.h"
#include "GL/glxtokens.h"
#include "sarea.h"
#include "xf86drm.h"
#include "dri.h"
#include "nv_dripriv.h"
#include "nv_dri.h"
#include "drmmode_display.h"

static Bool NVCreateContext(ScreenPtr pScreen, VisualPtr visual,
		drm_context_t hwContext, void *pVisualConfigPriv,
		DRIContextType contextStore)
{
	return TRUE;
}


static void NVDestroyContext(ScreenPtr pScreen, drm_context_t hwContext,
		DRIContextType contextStore)
{
	return;
}

static void NVDRISwapContext(ScreenPtr pScreen, DRISyncType syncType,
		DRIContextType oldContextType,
		void *oldContext,
		DRIContextType newContextType,
		void *newContext)
{
	/* we really should do something here */
	return;
}

static void NVDRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 indx)
{   
	return;
}

static void NVDRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg,
		RegionPtr prgnSrc, CARD32 indx)
{
	return;
}	

static void NVDRITransitionTo2d(ScreenPtr pScreen)
{
	return;
}

static void NVDRITransitionTo3d(ScreenPtr pScreen)
{
	return;
}

static void NVDRITransitionSingleToMulti3d(ScreenPtr pScreen)
{           
	return;
}           
        
static void NVDRITransitionMultiToSingle3d(ScreenPtr pScreen)
{       
	return;
}

static Bool NVDRIInitVisualConfigs(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn=xf86Screens[pScreen->myNum];
	__GLXvisualConfig* pConfigs = NULL;
	NVConfigPrivPtr pNVConfigs = NULL;
	NVConfigPrivPtr* pNVConfigPtrs = NULL;
	int db,depth,alpha,stencil;
	int depths[]={24,16,0};
	int num_configs,i;

	switch(pScrn->depth)
	{
		case 16:
		case 24:
			num_configs=2*3*((pScrn->depth==24)?2:1)*2; /* db*depth*alpha*stencil */
			if (!(pConfigs=(__GLXvisualConfig*)xcalloc(sizeof(__GLXvisualConfig),num_configs)))
				return FALSE;
			if (!(pNVConfigs=(NVConfigPrivPtr)xcalloc(sizeof(NVConfigPrivRec), num_configs))) {
				xfree(pConfigs);
				return FALSE;
			}
			if (!(pNVConfigPtrs=(NVConfigPrivPtr *)xcalloc(sizeof(NVConfigPrivPtr),num_configs))) {
				xfree(pConfigs);
				xfree(pNVConfigs);
				return FALSE;
			}

			i = 0;
			for(db=1;db>=0;db--)
			for(depth=0;depth<3;depth++)
			for(alpha=0;alpha<((pScrn->depth==24)?2:1);alpha++)
			for(stencil=0;stencil<2;stencil++)
			{
				pConfigs[i].vid                = (VisualID)(-1);
				pConfigs[i].class              = -1;
				pConfigs[i].rgba               = TRUE;
				if (pScrn->depth==16)
				{					
					pConfigs[i].redSize            = 5;
					pConfigs[i].greenSize          = 6;
					pConfigs[i].blueSize           = 5;
					pConfigs[i].alphaSize          = 0;
					pConfigs[i].redMask            = 0x0000F800;
					pConfigs[i].greenMask          = 0x000007E0;
					pConfigs[i].blueMask           = 0x0000001F;
					pConfigs[i].alphaMask          = 0x00000000;
				} else {
					pConfigs[i].redSize            = 8;
					pConfigs[i].greenSize          = 8;
					pConfigs[i].blueSize           = 8;
					pConfigs[i].redMask            = 0x00FF0000;
					pConfigs[i].greenMask          = 0x0000FF00;
					pConfigs[i].blueMask           = 0x000000FF;
					if (alpha) {
						pConfigs[i].alphaSize          = 8;
						pConfigs[i].alphaMask          = 0xFF000000;
					} else {
						pConfigs[i].alphaSize          = 0;
						pConfigs[i].alphaMask          = 0x00000000;
					}
				}

				pConfigs[i].accumRedSize   = 0;
				pConfigs[i].accumGreenSize = 0;
				pConfigs[i].accumBlueSize  = 0;
				pConfigs[i].accumAlphaSize = 0;
				if (db)
					pConfigs[i].doubleBuffer   = TRUE;
				else
					pConfigs[i].doubleBuffer   = FALSE;
				pConfigs[i].stereo             = FALSE;
				pConfigs[i].bufferSize         = pScrn->depth;
				if (depths[depth] == 24 && stencil) {
					pConfigs[i].depthSize          = depths[depth];
					pConfigs[i].stencilSize        = 8;
				} else {
					pConfigs[i].depthSize          = depths[depth];
					pConfigs[i].stencilSize        = 0;
				}
				pConfigs[i].auxBuffers         = 0;
				pConfigs[i].level              = 0;
				pConfigs[i].visualRating       = GLX_NONE;
				pConfigs[i].transparentPixel   = GLX_NONE;
				pConfigs[i].transparentRed     = 0;
				pConfigs[i].transparentGreen   = 0;
				pConfigs[i].transparentBlue    = 0;
				pConfigs[i].transparentAlpha   = 0;
				pConfigs[i].transparentIndex   = 0;
				i++;
			}
			break;
		default:
			xf86DrvMsg(pScreen->myNum, X_ERROR, "[dri] no DRI at %d bpp ",pScrn->depth);
			return FALSE;
	}
	GlxSetVisualConfigs(num_configs, pConfigs, (void**)pNVConfigPtrs);
	return TRUE;
}

Bool NVDRIGetVersion(ScrnInfoPtr pScrn)
{
	NVPtr pNv = NVPTR(pScrn);
	char *busId;
	int fd = 0;

#ifdef XF86DRM_MODE
	/* drm already open */
	if (pNv->drmmode) {
		drmmode_ptr drmmode = pNv->drmmode;
		fd = drmmode->fd;
	}
#endif

	{
		pointer ret;
		int errmaj, errmin;

		ret = LoadSubModule(pScrn->module, "dri", NULL, NULL, NULL,
				    NULL, &errmaj, &errmin);
		if (!ret) {
			xf86DrvMsg(pScrn->scrnIndex, X_INFO,
					"error %d\n", errmaj);
			LoaderErrorMsg(pScrn->name, "dri", errmaj, errmin);
		}

		if (!ret && errmaj != LDR_ONCEONLY)
			return FALSE;
	}

	xf86LoaderReqSymLists(drmSymbols, NULL);
	xf86LoaderReqSymLists(driSymbols, NULL);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Loaded DRI module\n");

	if (!fd) {
		busId = DRICreatePCIBusID(pNv->PciInfo);

		fd = drmOpen("nouveau", busId);
		xfree(busId);
	}
	if (fd < 0) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"[dri] Failed to open the DRM\n");
		return FALSE;
	}

	/* Check the lib version */
	if (xf86LoaderCheckSymbol("drmGetLibVersion"))
		pNv->pLibDRMVersion = drmGetLibVersion(0);
	if (pNv->pLibDRMVersion == NULL) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"NVDRIGetVersion failed because libDRM is really "
		"way to old to even get a version number out of it.\n"
		"[dri] Disabling DRI.\n");
		return FALSE;
	}

	pNv->pKernelDRMVersion = drmGetVersion(fd);
#ifdef XF86DRM_MODE
	if (!pNv->drmmode) /* drmmode still needs the file descriptor */
#endif
		drmClose(fd);

	if (pNv->pKernelDRMVersion == NULL) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"failed to get DRM version\n");
		return FALSE;
	}
	
	/* temporary lock step versioning */
#if NOUVEAU_DRM_HEADER_PATCHLEVEL != 10
#error nouveau_drm.h does not match expected patchlevel, update libdrm.
#endif
	if (pNv->pKernelDRMVersion->version_patchlevel !=
			NOUVEAU_DRM_HEADER_PATCHLEVEL) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"wrong DRM version\n");
		return FALSE;
	}

	return TRUE;
}

Bool NVDRICheckModules(ScrnInfoPtr pScrn)
{
	if (!xf86LoaderCheckSymbol("GlxSetVisualConfigs")) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "[dri] GlxSetVisualConfigs not found.\n");
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "      NVIDIA's glx present, or glx not loaded.\n");
		return FALSE;
	}

	return TRUE;
}

Bool NVDRIScreenInit(ScrnInfoPtr pScrn)
{
	DRIInfoPtr     pDRIInfo;
	NOUVEAUDRIPtr  pNOUVEAUDRI;
	NVPtr pNv = NVPTR(pScrn);
	ScreenPtr pScreen;
	pScreen = screenInfo.screens[pScrn->scrnIndex];
	int drm_page_size;
	int drm_fd;

#ifdef XF86DRM_MODE
	/* drm already open */
	if (pNv->drmmode) {
		drmmode_ptr drmmode = pNv->drmmode;
		drm_fd = drmmode->fd;
	}
#endif

	if (!NVDRICheckModules(pScrn))
		return FALSE;

	drm_page_size = getpagesize();
	if (!(pDRIInfo = DRICreateInfoRec())) return FALSE;
	
	pNv->pDRIInfo                        = pDRIInfo;
	pDRIInfo->drmDriverName              = "nouveau";
	pDRIInfo->clientDriverName           = "nouveau";
	pDRIInfo->busIdString                = DRICreatePCIBusID(pNv->PciInfo);

	pDRIInfo->ddxDriverMajorVersion      = NV_MAJOR_VERSION;
	pDRIInfo->ddxDriverMinorVersion      = NV_MINOR_VERSION;
	pDRIInfo->ddxDriverPatchVersion      = NV_PATCHLEVEL;

	/*
	 * We set the FB to be in the higher half of VRAM. If we don't, any
	 * VRAM allocations before the FB is mapped will change that map
	 * and we fail.
	 * We should detect when the DRM decides to change the FB area
	 * but we currently don't know how to.
	 */
	pDRIInfo->frameBufferSize            = pNv->VRAMPhysicalSize / 2;
	pDRIInfo->frameBufferPhysicalAddress = (void *)pNv->VRAMPhysical +
	                                       pDRIInfo->frameBufferSize;
	pDRIInfo->frameBufferStride          = pScrn->displayWidth * pScrn->bitsPerPixel/8;

	pDRIInfo->ddxDrawableTableEntry      = 1;
	pDRIInfo->maxDrawableTableEntry      = 1;

	if (!(pNOUVEAUDRI = (NOUVEAUDRIPtr)xcalloc(sizeof(NOUVEAUDRIRec), 1))) {
		DRIDestroyInfoRec(pDRIInfo);
		pNv->pDRIInfo = NULL;
		return FALSE;
	}
	pDRIInfo->devPrivate                 = pNOUVEAUDRI; 
	pDRIInfo->devPrivateSize             = sizeof(NOUVEAUDRIRec);
	pDRIInfo->contextSize                = sizeof(NVDRIContextRec);
	pDRIInfo->SAREASize                  = (drm_page_size > SAREA_MAX) ? drm_page_size : SAREA_MAX;

	pDRIInfo->CreateContext              = NVCreateContext;
	pDRIInfo->DestroyContext             = NVDestroyContext;
	pDRIInfo->SwapContext                = NVDRISwapContext;
	pDRIInfo->InitBuffers                = NVDRIInitBuffers;
	pDRIInfo->MoveBuffers                = NVDRIMoveBuffers;
	pDRIInfo->bufferRequests             = DRI_ALL_WINDOWS;
	pDRIInfo->TransitionTo2d             = NVDRITransitionTo2d;
	pDRIInfo->TransitionTo3d             = NVDRITransitionTo3d;
	pDRIInfo->TransitionSingleToMulti3D  = NVDRITransitionSingleToMulti3d;
	pDRIInfo->TransitionMultiToSingle3D  = NVDRITransitionMultiToSingle3d;

	pDRIInfo->createDummyCtx     = FALSE;
	pDRIInfo->createDummyCtxPriv = FALSE;

	if (!DRIScreenInit(pScreen, pDRIInfo, &drm_fd)) {
		xf86DrvMsg(pScreen->myNum, X_ERROR,
				"[dri] DRIScreenInit failed.  Disabling DRI.\n");
		xfree(pDRIInfo->devPrivate);
		pDRIInfo->devPrivate = NULL;
		DRIDestroyInfoRec(pDRIInfo);
		pDRIInfo = NULL;
		return FALSE;
	}

	if (!NVDRIInitVisualConfigs(pScreen)) {
		xf86DrvMsg(pScreen->myNum, X_ERROR,
				"[dri] NVDRIInitVisualConfigs failed.  Disabling DRI.\n");
		xfree(pDRIInfo->devPrivate);
		pDRIInfo->devPrivate = NULL;
		DRIDestroyInfoRec(pDRIInfo);
		pDRIInfo = NULL;
		return FALSE;
	}

	if (nouveau_device_open_existing(&pNv->dev, 0, drm_fd, 0)) {
		xf86DrvMsg(pScreen->myNum, X_ERROR, "Error creating device\n");
		xfree(pDRIInfo->devPrivate);
		pDRIInfo->devPrivate = NULL;
		DRIDestroyInfoRec(pDRIInfo);
		pDRIInfo = NULL;
		return FALSE;
	}

	return TRUE;
}

Bool NVDRIFinishScreenInit(ScrnInfoPtr pScrn)
{
	ScreenPtr      pScreen = screenInfo.screens[pScrn->scrnIndex];
	NVPtr          pNv = NVPTR(pScrn);
	NOUVEAUDRIPtr  pNOUVEAUDRI;

	if (!DRIFinishScreenInit(pScreen)) {
		return FALSE;
	}

	pNOUVEAUDRI 			= (NOUVEAUDRIPtr)pNv->pDRIInfo->devPrivate;

	pNOUVEAUDRI->device_id		= pNv->Chipset;

	pNOUVEAUDRI->width		= pScrn->virtualX;
	pNOUVEAUDRI->height		= pScrn->virtualY;
	pNOUVEAUDRI->depth		= pScrn->depth;
	pNOUVEAUDRI->bpp		= pScrn->bitsPerPixel;

	pNOUVEAUDRI->front_offset 	= pNv->FB->offset;
	pNOUVEAUDRI->front_pitch	= pScrn->displayWidth;
	/* back/depth buffers will likely be allocated on a per-drawable
	 * basis, but these may be useful if we want to support shared back
	 * buffers at some point.
	 */
	pNOUVEAUDRI->back_offset	= 0;
	pNOUVEAUDRI->back_pitch		= 0;
	pNOUVEAUDRI->depth_offset	= 0;
	pNOUVEAUDRI->depth_pitch	= 0;

	return TRUE;
}

