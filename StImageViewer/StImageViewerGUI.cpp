/**
 * Copyright © 2009-2015 Kirill Gavrilov <kirill@sview.ru>
 *
 * StImageViewer program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StImageViewer program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "StImageViewerGUI.h"
#include "StImageViewer.h"

#include <StCore/StWindow.h>

#include <StSettings/StEnumParam.h>
#include <StThreads/StThread.h>

#include <StGL/StParams.h>
#include <StGLWidgets/StGLAssignHotKey.h>
#include <StGLWidgets/StGLButton.h>
#include <StGLWidgets/StGLCombobox.h>
#include <StGLWidgets/StGLCheckboxTextured.h>
#include <StGLWidgets/StGLDescription.h>
#include <StGLWidgets/StGLImageRegion.h>
#include <StGLWidgets/StGLMenu.h>
#include <StGLWidgets/StGLMenuItem.h>
#include <StGLWidgets/StGLMsgStack.h>
#include <StGLWidgets/StGLOpenFile.h>
#include <StGLWidgets/StGLPlayList.h>
#include <StGLWidgets/StGLRangeFieldFloat32.h>
#include <StGLWidgets/StGLScrollArea.h>
#include <StGLWidgets/StGLSwitchTextured.h>
#include <StGLWidgets/StGLTable.h>
#include <StGLWidgets/StGLTextureButton.h>
#include <StGLWidgets/StGLFpsLabel.h>

#include <StImage/StImageFile.h>
#include <StVersion.h>

#include "StImageViewerStrings.h"

// auxiliary pre-processor definition
#define stCTexture(theString) getTexturePath(stCString(theString))
#define stCMenuIcon(theString) iconTexture(stCString(theString), myMenuIconSize)

using namespace StImageViewerStrings;

namespace {
    static const int DISPL_Y_REGION_UPPER = 32;
    static const int DISPL_X_REGION_UPPER = 32;
}

StInfoDialog::~StInfoDialog() {
    myPlugin->doSaveImageInfo(0);
}

void StImageViewerGUI::createDesktopUI(const StHandle<StPlayList>& thePlayList) {
    if(myPlugin->params.ToShowFps->getValue()) {
        myFpsWidget = new StGLFpsLabel(this);
    }

    createUpperToolbar();

    const StMarginsI& aRootMargins = getRootMargins();
    StMarginsI aButtonMargins;
    const IconSize anIconSize = scaleIcon(32, aButtonMargins);
    const int      anIconStep = scale(32);

    myPanelBottom = new StGLContainer(this, aRootMargins.left, -aRootMargins.bottom, StGLCorner(ST_VCORNER_BOTTOM, ST_HCORNER_LEFT), scale(4096), scale(32));
    int aBottomBarNbRight = 0;
    const int aRight  = -scale(8);
    const int aBottom = -scale(8);
    if(myWindow->hasFullscreenMode()) {
        myBtnFull = new StGLCheckboxTextured(myPanelBottom, myPlugin->params.isFullscreen,
                                             iconTexture(stCString("actionVideoFullscreenOff"), anIconSize),
                                             iconTexture(stCString("actionVideoFullscreenOn"),  anIconSize),
                                             (aBottomBarNbRight++) * (-anIconStep) + aRight, aBottom,
                                             StGLCorner(ST_VCORNER_TOP, ST_HCORNER_RIGHT));
        myBtnFull->setDrawShadow(true);
        myBtnFull->setFalseOpacity(1.0f);
        myBtnFull->changeMargins() = aButtonMargins;
    }

    myBtnList = new StGLCheckboxTextured(myPanelBottom, myPlugin->params.ToShowPlayList,
                                         iconTexture(stCString("actionVideoPlaylistOff"), anIconSize),
                                         iconTexture(stCString("actionVideoPlaylist"),    anIconSize),
                                         (aBottomBarNbRight++) * (-anIconStep) + aRight, aBottom,
                                         StGLCorner(ST_VCORNER_TOP, ST_HCORNER_RIGHT));
    myBtnList->setDrawShadow(true);
    myBtnList->changeMargins() = aButtonMargins;

    myDescr = new StGLDescription(this);

    myPlayList = new StGLPlayList(this, thePlayList);
    myPlayList->setCorner(StGLCorner(ST_VCORNER_TOP, ST_HCORNER_RIGHT));
    myPlayList->changeFitMargins().top    = scale(110);
    myPlayList->changeFitMargins().bottom = scale(110);
    //myPlayList->changeMargins().bottom    = scale(32);
    myPlayList->setOpacity(myPlugin->params.ToShowPlayList->getValue() ? 1.0f : 0.0f, false);
    myPlayList->signals.onOpenItem = stSlot(myPlugin, &StImageViewer::doFileNext);

    // create Main menu
    createMainMenu();
}

/**
 * Create upper toolbar
 */
void StImageViewerGUI::createUpperToolbar() {
    int aBtnIter = 0;
    const int aTop  = scale(DISPL_Y_REGION_UPPER);
    const int aLeft = scale(DISPL_X_REGION_UPPER);
    StMarginsI aButtonMargins;
    const IconSize anIconSize = scaleIcon(32, aButtonMargins);
    const int      anIconStep = scale(48);
    aButtonMargins.extend(scale(8));

    const StMarginsI& aMargins = getRootMargins();
    myPanelUpper = new StGLContainer(this, aMargins.left, aMargins.top, StGLCorner(ST_VCORNER_TOP, ST_HCORNER_LEFT), scale(4096), scale(128));

    // append textured buttons
    myBtnOpen   = new StGLTextureButton(myPanelUpper, aLeft + (aBtnIter++) * anIconStep, aTop);
    myBtnOpen->signals.onBtnClick.connect(myPlugin, &StImageViewer::doOpen1FileDialog);
    myBtnOpen->setTexturePath(iconTexture(stCString("actionOpen"), anIconSize));
    myBtnOpen->setDrawShadow(true);
    myBtnOpen->changeMargins() = aButtonMargins;

    myBtnPrev   = new StGLTextureButton(myPanelUpper, aLeft + (aBtnIter++) * anIconStep, aTop);
    myBtnPrev->signals.onBtnClick.connect(myPlugin, &StImageViewer::doListPrev);
    myBtnPrev->setTexturePath(iconTexture(stCString("actionBack"), anIconSize));
    myBtnPrev->setDrawShadow(true);
    myBtnPrev->changeMargins() = aButtonMargins;

    myBtnNext   = new StGLTextureButton(myPanelUpper, aLeft + (aBtnIter++) * anIconStep, aTop);
    myBtnNext->signals.onBtnClick.connect(myPlugin, &StImageViewer::doListNext);
    myBtnNext->setTexturePath(iconTexture(stCString("actionNext"), anIconSize));
    myBtnNext->setDrawShadow(true);
    myBtnNext->changeMargins() = aButtonMargins;

    myBtnInfo = new StGLTextureButton(myPanelUpper, aLeft + (aBtnIter++) * anIconStep, aTop);
    myBtnInfo->signals.onBtnClick += stSlot(myPlugin, &StImageViewer::doAboutImage);
    myBtnInfo->setTexturePath(iconTexture(stCString("actionInfo"),  anIconSize));
    myBtnInfo->setDrawShadow(true);
    myBtnInfo->changeMargins() = aButtonMargins;

    StGLTextureButton* aSrcBtn = new StGLTextureButton(myPanelUpper, aLeft + (aBtnIter++) * anIconStep, aTop,
                                                       StGLCorner(ST_VCORNER_TOP, ST_HCORNER_LEFT), StFormat_NB);
    aSrcBtn->changeMargins() = aButtonMargins;
    aSrcBtn->signals.onBtnClick += stSlot(this, &StImageViewerGUI::doDisplayStereoFormatCombo);
    const StString aSrcTextures[StFormat_NB] = {
        iconTexture(stCString("menuMono"),           anIconSize),
        iconTexture(stCString("menuSbsLR"),          anIconSize),
        iconTexture(stCString("menuSbsRL"),          anIconSize),
        iconTexture(stCString("menuOverUnderLR"),    anIconSize),
        iconTexture(stCString("menuOverUnderRL"),    anIconSize),
        iconTexture(stCString("menuRowLR"),          anIconSize),
        iconTexture(stCString("menuColLR"),          anIconSize),
        iconTexture(stCString("menuDual"),           anIconSize),
        iconTexture(stCString("menuFrameSeqLR"),     anIconSize),
        iconTexture(stCString("menuRedCyanLR"),      anIconSize),
        iconTexture(stCString("menuGreenMagentaLR"), anIconSize),
        iconTexture(stCString("menuYellowBlueLR"),   anIconSize),
        iconTexture(stCString("menuTiledLR"),        anIconSize)
    };
    aSrcBtn->setTexturePath(aSrcTextures, StFormat_NB);
    aSrcBtn->setDrawShadow(true);
    myBtnSrcFrmt = aSrcBtn;

    myBtnSwapLR = new StGLCheckboxTextured(myPanelUpper, myImage->params.swapLR,
                                           iconTexture(stCString("actionSwapLROff"), anIconSize),
                                           iconTexture(stCString("actionSwapLROn"),  anIconSize),
                                           aLeft + (aBtnIter++) * anIconStep, aTop,
                                           StGLCorner(ST_VCORNER_TOP, ST_HCORNER_LEFT));
    myBtnSwapLR->setDrawShadow(true);
    myBtnSwapLR->changeMargins() = aButtonMargins;

    StHandle<StBoolParam> aTrackedPano = new StBoolParam(false);
    myBtnPanorama = new StGLCheckboxTextured(myPanelUpper, aTrackedPano,
                                             iconTexture(stCString("actionPanoramaOff"), anIconSize),
                                             iconTexture(stCString("actionPanorama"),    anIconSize),
                                             aLeft + (aBtnIter++) * anIconStep, aTop,
                                             StGLCorner(ST_VCORNER_TOP, ST_HCORNER_LEFT));
    myBtnPanorama->signals.onBtnClick += stSlot(this, &StImageViewerGUI::doPanoramaCombo);
    myBtnPanorama->setDrawShadow(true);
    myBtnPanorama->changeMargins() = aButtonMargins;
}

/**
 * Main menu
 */
void StImageViewerGUI::createMainMenu() {
    const StMarginsI& aMargins = getRootMargins();
    myMenuRoot = new StGLMenu(this, aMargins.left, aMargins.top, StGLMenu::MENU_HORIZONTAL, true);

    StGLMenu* aMenuMedia   = createMediaMenu();  // Root -> Media  menu
    StGLMenu* aMenuView    = createViewMenu();   // Root -> View   menu
    StGLMenu* aDevicesMenu = createOutputMenu(); // Root -> Output menu
    StGLMenu* aMenuHelp    = createHelpMenu();   // Root -> Help   menu

    // Attach sub menus to root
    myMenuRoot->addItem(tr(MENU_MEDIA), aMenuMedia);
    myMenuRoot->addItem(tr(MENU_VIEW),  aMenuView);
    myMenuRoot->addItem(myPlugin->StApplication::params.ActiveDevice->getActiveValue(), aDevicesMenu);
    myMenuRoot->addItem(tr(MENU_HELP),  aMenuHelp);
}

/**
 * Root -> Media menu
 */
StGLMenu* StImageViewerGUI::createMediaMenu() {
    StGLMenu* aMenuMedia = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    StGLMenu* aMenuSrcFormat = createSrcFormatMenu(); // Root -> Media -> Source format menu
    StGLMenu* aMenuOpenImage = createOpenImageMenu(); // Root -> Media -> Open image menu
    StGLMenu* aMenuSaveImage = createSaveImageMenu(); // Root -> Media -> Save image menu

    aMenuMedia->addItem(tr(MENU_MEDIA_OPEN_IMAGE),    aMenuOpenImage)
              ->setIcon(stCMenuIcon("actionOpen"), false);
    aMenuMedia->addItem(tr(MENU_MEDIA_SAVE_IMAGE_AS), aMenuSaveImage)
              ->setIcon(stCMenuIcon("actionSave"), false);
    aMenuMedia->addItem(tr(MENU_MEDIA_SRC_FORMAT),    aMenuSrcFormat)
              ->setIcon(stCMenuIcon("actionSourceFormat"), false);
    aMenuMedia->addItem(tr(MENU_MEDIA_FILE_INFO),     myPlugin->getAction(StImageViewer::Action_FileInfo))
              ->setIcon(stCMenuIcon("actionInfo"), false);

    if(myWindow->isMobile()) {
        aMenuMedia->addItem("Mobile UI", myPlugin->params.IsMobileUI);
    }

    aMenuMedia->addItem(tr(MENU_MEDIA_QUIT))
              ->signals.onItemClick.connect(myPlugin, &StImageViewer::doQuit);
    return aMenuMedia;
}

/**
 * Root -> Media -> Open image menu
 */
StGLMenu* StImageViewerGUI::createOpenImageMenu() {
    StGLMenu* menu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    menu->addItem(tr(MENU_MEDIA_OPEN_IMAGE_1), 1)
        ->signals.onItemClick.connect(myPlugin, &StImageViewer::doOpen1FileDialog);
    menu->addItem(tr(MENU_MEDIA_OPEN_IMAGE_2), 2)
        ->signals.onItemClick.connect(myPlugin, &StImageViewer::doOpen2FilesDialog);
    return menu;
}

/**
 * Root -> Media -> Save image menu
 */
StGLMenu* StImageViewerGUI::createSaveImageMenu() {
    StGLMenu* menu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    menu->addItem("JPEG stereo (*.jps)", size_t(StImageFile::ST_TYPE_JPEG))
        ->signals.onItemClick.connect(myPlugin, &StImageViewer::doSaveImageAs);
    menu->addItem("PNG stereo (*.pns)", size_t(StImageFile::ST_TYPE_PNG))
        ->signals.onItemClick.connect(myPlugin, &StImageViewer::doSaveImageAs);
    return menu;
}

/**
 * Root -> Media -> Source format menu
 */
StGLMenu* StImageViewerGUI::createSrcFormatMenu() {
    StGLMenu* aMenu  = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    fillSrcFormatMenu(aMenu);
    return aMenu;
}

void StImageViewerGUI::fillSrcFormatMenu(StGLMenu* theMenu) {
    theMenu->addItem(tr(MENU_SRC_FORMAT_AUTO),         myPlugin->params.srcFormat, StFormat_AUTO)
           ->setIcon(stCMenuIcon("menuAuto"));
    theMenu->addItem(tr(MENU_SRC_FORMAT_MONO),         myPlugin->params.srcFormat, StFormat_Mono)
           ->setIcon(stCMenuIcon("menuMono"));
    theMenu->addItem(tr(MENU_SRC_FORMAT_PARALLEL),     myPlugin->params.srcFormat, StFormat_SideBySide_LR)
           ->setIcon(stCMenuIcon("menuSbsLR"));
    theMenu->addItem(tr(MENU_SRC_FORMAT_CROSS_EYED),   myPlugin->params.srcFormat, StFormat_SideBySide_RL)
           ->setIcon(stCMenuIcon("menuSbsRL"));
    theMenu->addItem(tr(MENU_SRC_FORMAT_OVERUNDER_LR), myPlugin->params.srcFormat, StFormat_TopBottom_LR)
           ->setIcon(stCMenuIcon("menuOverUnderLR"));
    theMenu->addItem(tr(MENU_SRC_FORMAT_OVERUNDER_RL), myPlugin->params.srcFormat, StFormat_TopBottom_RL)
           ->setIcon(stCMenuIcon("menuOverUnderRL"));
    theMenu->addItem(tr(MENU_SRC_FORMAT_INTERLACED),   myPlugin->params.srcFormat, StFormat_Rows)
           ->setIcon(stCMenuIcon("menuRowLR"));
    theMenu->addItem(tr(MENU_SRC_FORMAT_ANA_RC),       myPlugin->params.srcFormat, StFormat_AnaglyphRedCyan)
           ->setIcon(stCMenuIcon("menuRedCyanLR"));
    theMenu->addItem(tr(MENU_SRC_FORMAT_ANA_RB),       myPlugin->params.srcFormat, StFormat_AnaglyphGreenMagenta)
           ->setIcon(stCMenuIcon("menuGreenMagentaLR"));
    theMenu->addItem(tr(MENU_SRC_FORMAT_ANA_YB),       myPlugin->params.srcFormat, StFormat_AnaglyphYellowBlue)
           ->setIcon(stCMenuIcon("menuYellowBlueLR"));
}

void StImageViewerGUI::doDisplayStereoFormatCombo(const size_t ) {
    StGLCombobox::ListBuilder aBuilder(this);
    fillSrcFormatMenu(aBuilder.getMenu());
    aBuilder.display();
}

/**
 * Root -> View menu
 */
StGLMenu* StImageViewerGUI::createViewMenu() {
    StGLMenu* aMenuView = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    StGLMenu* aMenuDispMode  = createDisplayModeMenu();
    StGLMenu* aMenuDispRatio = createDisplayRatioMenu();
    StGLMenu* aMenuPanorama  = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    fillPanoramaMenu(aMenuPanorama);
    StGLMenu* aMenuTexFilter = createSmoothFilterMenu();
    StGLMenu* aMenuImgAdjust = createImageAdjustMenu();

    aMenuView->addItem(tr(MENU_VIEW_DISPLAY_MODE),  aMenuDispMode);
    if(myWindow->hasFullscreenMode()) {
        aMenuView->addItem(tr(MENU_VIEW_FULLSCREEN),    myPlugin->params.isFullscreen);
    }
    aMenuView->addItem(tr(MENU_VIEW_RESET))
             ->setIcon(stCMenuIcon("actionResetPlacement"), false)
             ->signals.onItemClick.connect(myPlugin, &StImageViewer::doReset);
    aMenuView->addItem(tr(MENU_VIEW_SWAP_LR),       myImage->params.swapLR);
    aMenuView->addItem(tr(MENU_VIEW_DISPLAY_RATIO), aMenuDispRatio)
             ->setIcon(stCMenuIcon("actionDisplayRatio"), false);
    aMenuView->addItem(tr(MENU_VIEW_PANORAMA),      aMenuPanorama)
             ->setIcon(stCMenuIcon("actionPanorama"), false);
    aMenuView->addItem(tr(MENU_VIEW_TEXFILTER),     aMenuTexFilter)
             ->setIcon(stCMenuIcon("actionInterpolation"), false);
    aMenuView->addItem(tr(MENU_VIEW_IMAGE_ADJUST),  aMenuImgAdjust)
             ->setIcon(stCMenuIcon("actionColorAdjust"), false);
    return aMenuView;
}

/**
 * Root -> View menu -> Output
 */
StGLMenu* StImageViewerGUI::createDisplayModeMenu() {
    StGLMenu* aMenu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    const StArrayList<StString>& aValuesList = myImage->params.displayMode->getValues();
    for(size_t aValIter = 0; aValIter < aValuesList.size(); ++aValIter) {
        aMenu->addItem(aValuesList[aValIter], myImage->params.displayMode, int32_t(aValIter));
    }
    return aMenu;
}

/**
 * Root -> View menu -> Display Ratio
 */
StGLMenu* StImageViewerGUI::createDisplayRatioMenu() {
    StGLMenu* aMenu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    aMenu->addItem(tr(MENU_VIEW_DISPLAY_RATIO_SRC), myImage->params.displayRatio, StGLImageRegion::RATIO_AUTO)
         ->setIcon(stCMenuIcon("menuAuto"));
    aMenu->addItem("2.21:1", myImage->params.displayRatio, StGLImageRegion::RATIO_221_1)
         ->setIcon(stCMenuIcon("menuRatio2_1_"));
    aMenu->addItem("16:9",   myImage->params.displayRatio, StGLImageRegion::RATIO_16_9)
         ->setIcon(stCMenuIcon("menuRatio16_9_"));
    aMenu->addItem("16:10",  myImage->params.displayRatio, StGLImageRegion::RATIO_16_10)
         ->setIcon(stCMenuIcon("menuRatio16_10_"));
    aMenu->addItem("4:3",    myImage->params.displayRatio, StGLImageRegion::RATIO_4_3)
         ->setIcon(stCMenuIcon("menuRatio4_3_"));
    aMenu->addItem("5:4",    myImage->params.displayRatio, StGLImageRegion::RATIO_5_4)
         ->setIcon(stCMenuIcon("menuRatio5_4_"));
    aMenu->addItem("1:1",    myImage->params.displayRatio, StGLImageRegion::RATIO_1_1)
         ->setIcon(stCMenuIcon("menuRatio1_1_"));
    aMenu->addItem(tr(MENU_VIEW_RATIO_KEEP_ON_RESTART), myPlugin->params.toRestoreRatio);
    aMenu->addItem(tr(MENU_VIEW_RATIO_HEAL_ANAMORPHIC), myImage->params.ToHealAnamorphicRatio);
    return aMenu;
}

void StImageViewerGUI::fillPanoramaMenu(StGLMenu* theMenu) {
    theMenu->addItem(tr(MENU_VIEW_SURFACE_PLANE),
                     myImage->params.ViewMode, StStereoParams::FLAT_IMAGE);
    theMenu->addItem(tr(MENU_VIEW_SURFACE_SPHERE),
                     myImage->params.ViewMode, StStereoParams::PANORAMA_SPHERE);
    theMenu->addItem(tr(MENU_VIEW_SURFACE_CUBEMAP),
                     myImage->params.ViewMode, StStereoParams::PANORAMA_CUBEMAP);
    if(myWindow->hasOrientationSensor()) {
        theMenu->addItem(tr(myWindow->isPoorOrientationSensor() ? MENU_VIEW_TRACK_HEAD_POOR : MENU_VIEW_TRACK_HEAD),
                         myPlugin->params.ToTrackHead);
    }
}

void StImageViewerGUI::doPanoramaCombo(const size_t ) {
    StGLCombobox::ListBuilder aBuilder(this);
    fillPanoramaMenu(aBuilder.getMenu());
    aBuilder.display();
}

/**
 * Root -> View menu -> Smooth Filter
 */
StGLMenu* StImageViewerGUI::createSmoothFilterMenu() {
    StGLMenu* aMenu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    aMenu->addItem(tr(MENU_VIEW_TEXFILTER_NEAREST),
                   myImage->params.textureFilter, StGLImageProgram::FILTER_NEAREST);
    aMenu->addItem(tr(MENU_VIEW_TEXFILTER_LINEAR),
                   myImage->params.textureFilter, StGLImageProgram::FILTER_LINEAR);
    return aMenu;
}

/**
 * Root -> View menu -> Image Adjust
 */
StGLMenu* StImageViewerGUI::createImageAdjustMenu() {
    StGLMenu* aMenu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);

    const StGLVec3 aBlack(0.0f, 0.0f, 0.0f);
    const StGLVec3 aGreen(0.0f, 0.6f, 0.4f);
    const StGLVec3 aRed  (1.0f, 0.0f, 0.0f);

    aMenu->addItem(tr(MENU_VIEW_ADJUST_RESET), myPlugin->getAction(StImageViewer::Action_ImageAdjustReset))
         ->setIcon(stCMenuIcon("actionColorReset"), false);

    StGLMenuItem* anItem = aMenu->addItem(tr(MENU_VIEW_ADJUST_GAMMA));
    anItem->changeMargins().right = scale(100 + 16);
    StGLRangeFieldFloat32* aRange = new StGLRangeFieldFloat32(anItem, myImage->params.gamma,
                                                              -scale(16), 0, StGLCorner(ST_VCORNER_CENTER, ST_HCORNER_RIGHT));
    aRange->setFormat(stCString("%+01.2f"));
    aRange->setColor(StGLRangeFieldFloat32::FieldColor_Default,  aBlack);
    aRange->setColor(StGLRangeFieldFloat32::FieldColor_Positive, aGreen);
    aRange->setColor(StGLRangeFieldFloat32::FieldColor_Negative, aRed);

    anItem = aMenu->addItem(tr(MENU_VIEW_ADJUST_BRIGHTNESS));
    anItem->setIcon(stCMenuIcon("actionBrightness"), false);
    anItem->changeMargins().right = scale(100 + 16);
    aRange = new StGLRangeFieldFloat32(anItem, myImage->params.brightness,
                                       -scale(16), 0, StGLCorner(ST_VCORNER_CENTER, ST_HCORNER_RIGHT));
    aRange->setFormat(stCString("%+01.2f"));
    aRange->setColor(StGLRangeFieldFloat32::FieldColor_Default,  aBlack);
    aRange->setColor(StGLRangeFieldFloat32::FieldColor_Positive, aGreen);
    aRange->setColor(StGLRangeFieldFloat32::FieldColor_Negative, aRed);

    anItem = aMenu->addItem(tr(MENU_VIEW_ADJUST_SATURATION));
    anItem->setIcon(stCMenuIcon("actionSaturation"), false);
    anItem->changeMargins().right = scale(100 + 16);
    aRange = new StGLRangeFieldFloat32(anItem, myImage->params.saturation,
                                       -scale(16), 0, StGLCorner(ST_VCORNER_CENTER, ST_HCORNER_RIGHT));
    aRange->changeRectPx().bottom() = aRange->getRectPx().top() + aMenu->getItemHeight();
    aRange->setFormat(stCString("%+01.2f"));
    aRange->setColor(StGLRangeFieldFloat32::FieldColor_Default,  aBlack);
    aRange->setColor(StGLRangeFieldFloat32::FieldColor_Positive, aGreen);
    aRange->setColor(StGLRangeFieldFloat32::FieldColor_Negative, aRed);
    return aMenu;
}

/**
 * Root -> Output menu
 */
StGLMenu* StImageViewerGUI::createOutputMenu() {
    StGLMenu* aMenu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);

    StGLMenu* aMenuChangeDevice = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);

    const StHandle<StEnumParam>& aDevicesEnum = myPlugin->StApplication::params.ActiveDevice;
    const StArrayList<StString>& aValuesList  = aDevicesEnum->getValues();
    for(size_t aValIter = 0; aValIter < aValuesList.size(); ++aValIter) {
        aMenuChangeDevice->addItem(aValuesList[aValIter], aDevicesEnum, int32_t(aValIter));
    }

    aMenu->addItem(tr(MENU_CHANGE_DEVICE), aMenuChangeDevice);
    aMenu->addItem(tr(MENU_ABOUT_RENDERER))
         ->setIcon(stCMenuIcon("actionHelp"), false)
         ->signals.onItemClick.connect(this, &StImageViewerGUI::doAboutRenderer);
    aMenu->addItem(tr(MENU_SHOW_FPS),      myPlugin->params.ToShowFps);
    aMenu->addItem(tr(MENU_VSYNC),         myPlugin->params.IsVSyncOn);

    const StHandle<StWindow>& aRend = myPlugin->getMainWindow();
    StParamsList aParams;
    aRend->getOptions(aParams);
    StHandle<StBoolParamNamed> aBool;
    StHandle<StEnumParam>      anEnum;
    for(size_t anIter = 0; anIter < aParams.size(); ++anIter) {
        const StHandle<StParamBase>& aParam = aParams[anIter];
        if(aBool.downcastFrom(aParam)) {
            aMenu->addItem(aBool->getName(), aBool);
        } else if(anEnum.downcastFrom(aParam)) {
            StGLMenu* aChildMenu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
            const StArrayList<StString>& aValues = anEnum->getValues();
            for(size_t aValIter = 0; aValIter < aValues.size(); ++aValIter) {
                aChildMenu->addItem(aValues[aValIter], anEnum, int32_t(aValIter));
            }
            aMenu->addItem(anEnum->getName(), aChildMenu);
        }
    }
    return aMenu;
}

void StImageViewerGUI::doAboutProgram(const size_t ) {
    const StGLVec3 THE_WHITE(1.0f, 1.0f, 1.0f);
    const StString anAbout = tr(ABOUT_DPLUGIN_NAME) + '\n'
                           + tr(ABOUT_VERSION) + " " + StVersionInfo::getSDKVersionString()
                           + "\n \n" + tr(ABOUT_DESCRIPTION).format("2007-2015", "kirill@sview.ru", "www.sview.ru");

    StArgumentsMap anInfo;
    anInfo.add(StDictEntry("CPU cores", StString(StThread::countLogicalProcessors()) + StString(" logical processor(s)")));
    getContext().stglFullInfo(anInfo);

    StGLMessageBox* aDialog = new StGLMessageBox(this, tr(MENU_HELP_ABOUT), "", scale(512), scale(300));
    StGLTable* aTable = new StGLTable(aDialog->getContent(), 0, 0, StGLCorner(ST_VCORNER_TOP, ST_HCORNER_CENTER));
    aTable->setupTable((int )anInfo.size() + 1, 2);

    const int aTextMaxWidth = aDialog->getContent()->getRectPx().width() - 2 * (aTable->getItemMargins().left + aTable->getItemMargins().right);
    StGLTableItem& anAboutItem = aTable->changeElement(0, 0); anAboutItem.setColSpan(2);
    StGLTextArea*  anAboutLab  = new StGLTextArea(&anAboutItem, 0, 0, StGLCorner(ST_VCORNER_CENTER, ST_HCORNER_CENTER));
    anAboutLab->setupAlignment(StGLTextFormatter::ST_ALIGN_X_CENTER,
                               StGLTextFormatter::ST_ALIGN_Y_TOP);
    anAboutLab->setText(anAbout + "\n\n<b><i>" + tr(ABOUT_SYSTEM) + "</i></b>\n");
    anAboutLab->setTextColor(THE_WHITE);
    anAboutLab->stglInitAutoHeightWidth(aTextMaxWidth);

    aTable->fillFromMap(anInfo, THE_WHITE,
                        aDialog->getContent()->getRectPx().width(),
                        aDialog->getContent()->getRectPx().width() / 2, 1);

    aDialog->addButton(tr(BUTTON_CLOSE));
    aDialog->stglInit();
}

void StImageViewerGUI::doUserTips(const size_t ) {
    StProcess::openURL("http://sview.ru/sview/usertips");
}

void StImageViewerGUI::doAboutImage(const size_t ) {
    StHandle<StImageInfo>& anExtraInfo = myPlugin->myFileInfo;
    if(!anExtraInfo.isNull()) {
        return; // already opened
    }

    StHandle<StFileNode>     aFileNode;
    StHandle<StStereoParams> aParams;
    if(!myPlugin->getCurrentFile(aFileNode, aParams, anExtraInfo)
    ||  anExtraInfo.isNull()) {
        StHandle<StMsgQueue> aQueue = myPlugin->getMessagesQueue();
        aQueue->pushInfo(tr(DIALOG_FILE_NOINFO));
        anExtraInfo.nullify();
        return;
    }

    const StString aTitle  = tr(DIALOG_FILE_INFO);
    StInfoDialog*  aDialog = new StInfoDialog(myPlugin, this, aTitle, scale(512), scale(300));

    // translate known metadata tag names
    for(size_t aMapIter = 0; aMapIter < anExtraInfo->Info.size(); ++aMapIter) {
        StDictEntry& anEntry = anExtraInfo->Info.changeValue(aMapIter);
        anEntry.changeName() = myLangMap->getValue(anEntry.getKey());
    }
    const int aWidthMax  = aDialog->getContent()->getRectPx().width();
    int       aRowLast   = (int )anExtraInfo->Info.size();
    const int aNbRowsMax = aRowLast + 2;

    StGLTable* aTable = new StGLTable(aDialog->getContent(), 0, 0, StGLCorner(ST_VCORNER_TOP, ST_HCORNER_CENTER));
    aTable->setupTable(aNbRowsMax, 2);
    aTable->fillFromMap(anExtraInfo->Info, StGLVec3(1.0f, 1.0f, 1.0f), aWidthMax, aWidthMax / 2);

    // add stereoscopic format info
    const StFormat anActiveSrcFormat = aParams->ToSwapLR
                                     ? st::formatReversed(aParams->StereoFormat)
                                     : aParams->StereoFormat;
    const int aTextMaxWidth = aWidthMax - (aTable->getItemMargins().left + aTable->getItemMargins().right);
    StGLTableItem& aSrcFormatItem = aTable->changeElement(aRowLast++, 0); aSrcFormatItem.setColSpan(2);
    StGLTextArea*  aSrcFormatText = new StGLTextArea(&aSrcFormatItem, 0, 0, StGLCorner(ST_VCORNER_CENTER, ST_HCORNER_CENTER));
    aSrcFormatText->setupAlignment(StGLTextFormatter::ST_ALIGN_X_CENTER,
                                   StGLTextFormatter::ST_ALIGN_Y_TOP);
    aSrcFormatText->setText(StString("\n") + tr(BTN_SRC_FORMAT) + " " + trSrcFormat(anActiveSrcFormat));
    aSrcFormatText->setTextColor(StGLVec3(1.0f, 1.0f, 1.0f));
    aSrcFormatText->stglInitAutoHeightWidth(aTextMaxWidth);

    // warn about wrong/missing stereoscopic format information
    StString aSrcInfo;
    StGLVec3 anExtraColor(1.0f, 1.0f, 1.0f);
    if(anExtraInfo->StInfoStream == StFormat_AUTO
    && anActiveSrcFormat != StFormat_Mono
    && anActiveSrcFormat != StFormat_SeparateFrames) {
        anExtraColor = StGLVec3(1.0f, 1.0f, 0.8f);
        if(anExtraInfo->StInfoFileName != StFormat_AUTO
        && anExtraInfo->StInfoFileName == anActiveSrcFormat) {
            aSrcInfo = tr(INFO_NO_SRCFORMAT_EX);
        } else {
            aSrcInfo = tr(INFO_NO_SRCFORMAT);
        }
    } else if(anExtraInfo->StInfoStream != StFormat_AUTO
           && anExtraInfo->StInfoStream != anActiveSrcFormat) {
        aSrcInfo     = tr(INFO_WRONG_SRCFORMAT);
        anExtraColor = StGLVec3(1.0f, 0.0f, 0.0f);
    }
    if(!aSrcInfo.isEmpty()) {
        StGLTableItem& aTabItem = aTable->changeElement(aRowLast++, 0); aTabItem.setColSpan(2);
        StGLTextArea*  aText    = new StGLTextArea(&aTabItem, 0, 0, StGLCorner(ST_VCORNER_CENTER, ST_HCORNER_CENTER));
        aText->setupAlignment(StGLTextFormatter::ST_ALIGN_X_CENTER,
                              StGLTextFormatter::ST_ALIGN_Y_TOP);
        aText->setText(aSrcInfo);
        aText->setTextColor(anExtraColor);
        aText->stglInitAutoHeightWidth(aTextMaxWidth);
    }
    aTable->updateLayout();

    if(anExtraInfo->IsSavable
    && !aSrcInfo.isEmpty()) {
        StGLButton* aSaveBtn = aDialog->addButton(tr(BUTTON_SAVE_METADATA));
        aSaveBtn->setUserData(1);
        aSaveBtn->signals.onBtnClick += stSlot(myPlugin, &StImageViewer::doSaveImageInfo);
    }

    aDialog->addButton(tr(BUTTON_CLOSE), true);
    aDialog->stglInit();
}

/**
 * Widget to assign new hot key.
 */
class ST_LOCAL StHotKeyControl : public StGLAssignHotKey {

        public:

    /**
     * Main constructor.
     */
    ST_LOCAL StHotKeyControl(StApplication*            thePlugin,
                             StGLTable*                theHKeysTable,
                             StImageViewerGUI*         theParent,
                             const StHandle<StAction>& theAction,
                             const int                 theHKeyIndex)
    : StGLAssignHotKey(theParent, theAction, theHKeyIndex),
      myPlugin(thePlugin),
      myHKeysTable(theHKeysTable) {
        myTitleFrmt    = theParent->tr(DIALOG_ASSIGN_HOT_KEY).format(theParent->tr(theAction->getName()));
        myConflictFrmt = theParent->tr(DIALOG_CONFLICTS_WITH);
        myAssignLab    = theParent->tr(BUTTON_ASSIGN);
        myDefaultLab   = theParent->tr(BUTTON_DEFAULT);
        myCancelLab    = theParent->tr(BUTTON_CANCEL);
        create();
    }

    /**
     * Destructor.
     */
    ST_LOCAL virtual ~StHotKeyControl() {
        myPlugin->registerHotKeys();
        myHKeysTable->updateHotKeys(myPlugin->getActions());
    }

    /**
     * Retrieve action for specified hot key.
     */
    ST_LOCAL virtual StHandle<StAction> getActionForKey(unsigned int theHKey) const {
        return myPlugin->getActionForKey(theHKey);
    }

        private:

    StApplication* myPlugin;
    StGLTable*     myHKeysTable;

};

void StImageViewerGUI::doResetHotKeys(const size_t ) {
    if(myHKeysTable == NULL) {
        return;
    }

    for(std::map< int, StHandle<StAction> >::iterator anActionIter = myPlugin->changeActions().begin();
        anActionIter != myPlugin->changeActions().end(); ++anActionIter) {
        StHandle<StAction>& anAction = anActionIter->second;
        anAction->setHotKey1(anAction->getDefaultHotKey1());
        anAction->setHotKey2(anAction->getDefaultHotKey2());
    }
    myPlugin->registerHotKeys();
    myHKeysTable->updateHotKeys(myPlugin->getActions());
}

void StImageViewerGUI::doListHotKeys(const size_t ) {
    const StHandle<StWindow>& aRend = myPlugin->getMainWindow();
    StParamsList aParams;
    aParams.add(myPlugin->StApplication::params.ActiveDevice);
    aParams.add(myImage->params.displayMode);
    aRend->getOptions(aParams);
    aParams.add(myPlugin->params.ToShowFps);
    aParams.add(myLangMap->params.language);
    aParams.add(myPlugin->params.IsMobileUI);
    myLangMap->params.language->setName(tr(MENU_HELP_LANGS));

    const StString aTitle  = tr(MENU_HELP_HOTKEYS);
    StInfoDialog*  aDialog = new StInfoDialog(myPlugin, this, aTitle, scale(650), scale(300));

    std::map< int, StHandle<StAction> >& anActionsMap = myPlugin->changeActions();

    StGLTable* aTable = new StGLTable(aDialog->getContent(), 0, 0, StGLCorner(ST_VCORNER_TOP, ST_HCORNER_CENTER));
    aTable->changeItemMargins().top    = scale(4);
    aTable->changeItemMargins().bottom = scale(4);
    aTable->setupTable((int )anActionsMap.size(), 3);

    StHandle< StSlot<void (const size_t )> > aSlot1 = new StSlotMethod<StImageViewerGUI, void (const size_t )>(this, &StImageViewerGUI::doChangeHotKey1);
    StHandle< StSlot<void (const size_t )> > aSlot2 = new StSlotMethod<StImageViewerGUI, void (const size_t )>(this, &StImageViewerGUI::doChangeHotKey2);
    aTable->fillFromHotKeys(anActionsMap, *myLangMap, aSlot1, aSlot2);
    myHKeysTable = aTable;

    aDialog->addButton(tr(BUTTON_DEFAULTS), false)->signals.onBtnClick = stSlot(this, &StImageViewerGUI::doResetHotKeys);
    aDialog->addButton(tr(BUTTON_CLOSE),    true);
    aDialog->stglInit();
}

void StImageViewerGUI::doChangeHotKey1(const size_t theId) {
    const StHandle<StAction>& anAction = myPlugin->getAction((int )theId);
    StHotKeyControl* aKeyChanger = new StHotKeyControl(myPlugin, myHKeysTable, this, anAction, 1);
    aKeyChanger->stglInit();
}

void StImageViewerGUI::doChangeHotKey2(const size_t theId) {
    const StHandle<StAction>& anAction = myPlugin->getAction((int )theId);
    StHotKeyControl* aKeyChanger = new StHotKeyControl(myPlugin, myHKeysTable, this, anAction, 2);
    aKeyChanger->stglInit();
}

void StImageViewerGUI::doMobileSettings(const size_t ) {
    const StHandle<StWindow>& aRend = myPlugin->getMainWindow();
    StParamsList aParams;
    aParams.add(myPlugin->StApplication::params.ActiveDevice);
    aParams.add(myImage->params.displayMode);
    aRend->getOptions(aParams);
    aParams.add(myPlugin->params.ToShowFps);
    aParams.add(myLangMap->params.language);
    aParams.add(myPlugin->params.IsMobileUI);
    myLangMap->params.language->setName(tr(MENU_HELP_LANGS));

    StInfoDialog* aDialog = new StInfoDialog(myPlugin, this, tr(MENU_HELP_SETTINGS), scale(512), scale(300));

    const int aWidthMax  = aDialog->getContent()->getRectPx().width();
    int       aRowLast   = (int )aParams.size();
    const int aNbRowsMax = aRowLast + 2;

    StGLTable* aTable = new StGLTable(aDialog->getContent(), 0, 0, StGLCorner(ST_VCORNER_TOP, ST_HCORNER_CENTER));
    aTable->changeItemMargins().top    = scale(4);
    aTable->changeItemMargins().bottom = scale(4);
    aTable->setupTable(aNbRowsMax, 2);
    aTable->fillFromParams(aParams, StGLVec3(1.0f, 1.0f, 1.0f), aWidthMax);

    aDialog->addButton(tr(BUTTON_CLOSE), true);
    aDialog->stglInit();
}

void StImageViewerGUI::doCheckUpdates(const size_t ) {
    StProcess::openURL("http://www.sview.ru/download");
}

void StImageViewerGUI::doOpenLicense(const size_t ) {
    StProcess::openURL(StProcess::getStShareFolder()
                       + "info" + SYS_FS_SPLITTER
                       + "license.txt");
}

/**
 * Root -> Help menu
 */
StGLMenu* StImageViewerGUI::createHelpMenu() {
    StGLMenu* aMenu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    StGLMenu* aMenuScale        = createScaleMenu();        // Root -> Help -> Scale Interface menu
#if !defined(ST_NO_UPDATES_CHECK)
    StGLMenu* aMenuCheckUpdates = createCheckUpdatesMenu(); // Root -> Help -> Check updates menu
#endif
    StGLMenu* aMenuLanguage     = createLanguageMenu();     // Root -> Help -> Language menu

    aMenu->addItem(tr(MENU_HELP_ABOUT))
         ->setIcon(stCMenuIcon("actionHelp"), false)
         ->signals.onItemClick.connect(this, &StImageViewerGUI::doAboutProgram);
    aMenu->addItem(tr(MENU_HELP_USERTIPS))
         ->signals.onItemClick.connect(this, &StImageViewerGUI::doUserTips);
    aMenu->addItem(tr(MENU_HELP_HOTKEYS))
         ->setIcon(stCMenuIcon("actionKeyboard"), false)
         ->signals.onItemClick.connect(this, &StImageViewerGUI::doListHotKeys);
    aMenu->addItem(tr(MENU_HELP_LICENSE))
         ->signals.onItemClick.connect(this, &StImageViewerGUI::doOpenLicense);
    aMenu->addItem(tr(MENU_HELP_SCALE),   aMenuScale)
         ->setIcon(stCMenuIcon("actionFontSize"), false);
#if !defined(ST_NO_UPDATES_CHECK)
    aMenu->addItem(tr(MENU_HELP_UPDATES), aMenuCheckUpdates);
#endif
    aMenu->addItem(tr(MENU_HELP_LANGS),   aMenuLanguage)
         ->setIcon(stCMenuIcon("actionLanguage"), false);
    return aMenu;
}

/**
 * Root -> Help -> Scale Interface menu
 */
StGLMenu* StImageViewerGUI::createScaleMenu() {
    StGLMenu* aMenu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    aMenu->addItem(tr(MENU_HELP_SCALE_SMALL),   myPlugin->params.ScaleAdjust,  StGLRootWidget::ScaleAdjust_Small);
    aMenu->addItem(tr(MENU_HELP_SCALE_NORMAL),  myPlugin->params.ScaleAdjust,  StGLRootWidget::ScaleAdjust_Normal);
    aMenu->addItem(tr(MENU_HELP_SCALE_BIG),     myPlugin->params.ScaleAdjust,  StGLRootWidget::ScaleAdjust_Big);
    aMenu->addItem(tr(MENU_HELP_SCALE_HIDPI2X), myPlugin->params.ScaleHiDPI2X);
    aMenu->addItem("Mobile UI",                 myPlugin->params.IsMobileUI);
    return aMenu;
}

/**
 * Root -> Help -> Check updates menu
 */
StGLMenu* StImageViewerGUI::createCheckUpdatesMenu() {
    StGLMenu* aMenu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    aMenu->addItem(tr(MENU_HELP_UPDATES_NOW))
         ->signals.onItemClick.connect(this, &StImageViewerGUI::doCheckUpdates);
    aMenu->addItem(tr(MENU_HELP_UPDATES_DAY),   myPlugin->params.checkUpdatesDays, 1);
    aMenu->addItem(tr(MENU_HELP_UPDATES_WEEK),  myPlugin->params.checkUpdatesDays, 7);
    aMenu->addItem(tr(MENU_HELP_UPDATES_YEAR),  myPlugin->params.checkUpdatesDays, 355);
    aMenu->addItem(tr(MENU_HELP_UPDATES_NEVER), myPlugin->params.checkUpdatesDays, 0);
    return aMenu;
}

/**
 * Root -> Help -> Language menu
 */
StGLMenu* StImageViewerGUI::createLanguageMenu() {
    StGLMenu* aMenu = new StGLMenu(this, 0, 0, StGLMenu::MENU_VERTICAL);
    for(size_t aLangId = 0; aLangId < myLangMap->getLanguagesList().size(); ++aLangId) {
        aMenu->addItem(myLangMap->getLanguagesList()[aLangId], myLangMap->params.language, int32_t(aLangId));
    }
    return aMenu;
}

void StImageViewerGUI::createMobileUI(const StHandle<StPlayList>& thePlayList) {
    createMobileUpperToolbar();
    createMobileBottomToolbar();

    myPlayList = new StGLPlayList(this, thePlayList);
    myPlayList->setCorner(StGLCorner(ST_VCORNER_TOP, ST_HCORNER_RIGHT));
    myPlayList->changeFitMargins().top    = scale(56);
    myPlayList->changeFitMargins().bottom = scale(100);
    //myPlayList->changeMargins().bottom    = scale(56);
    myPlayList->setOpacity(myPlugin->params.ToShowPlayList->getValue() ? 1.0f : 0.0f, false);
    myPlayList->signals.onOpenItem = stSlot(myPlugin, &StImageViewer::doFileNext);

    if(myPlugin->params.ToShowFps->getValue()) {
        myFpsWidget = new StGLFpsLabel(this);
    }
}

/**
 * Create upper toolbar
 */
void StImageViewerGUI::createMobileUpperToolbar() {
    StMarginsI aButtonMargins;
    const IconSize anIconSize = scaleIcon(32, aButtonMargins);
    const int      anIconStep = scale(56);
    aButtonMargins.extend(scale(12));

    const StMarginsI& aRootMargins = getRootMargins();
    myPanelUpper = new StGLContainer(this, aRootMargins.left, aRootMargins.top, StGLCorner(ST_VCORNER_TOP, ST_HCORNER_LEFT), scale(4096), scale(56));

    int aBtnIter = 0;

    myBtnOpen = new StGLTextureButton(myPanelUpper, (aBtnIter++) * anIconStep, 0);
    myBtnOpen->signals.onBtnClick.connect(this, &StImageViewerGUI::doOpenFile);
    myBtnOpen->setTexturePath(iconTexture(stCString("actionOpen"), anIconSize));
    myBtnOpen->setDrawShadow(true);
    myBtnOpen->changeMargins() = aButtonMargins;

    StGLTextureButton* aSrcBtn = new StGLTextureButton(myPanelUpper, (aBtnIter++) * anIconStep, 0,
                                                       StGLCorner(ST_VCORNER_TOP, ST_HCORNER_LEFT), StFormat_NB);
    aSrcBtn->changeMargins() = aButtonMargins;
    aSrcBtn->signals.onBtnClick += stSlot(this, &StImageViewerGUI::doDisplayStereoFormatCombo);
    const StString aSrcTextures[StFormat_NB] = {
        iconTexture(stCString("menuMono"),           anIconSize),
        iconTexture(stCString("menuSbsLR"),          anIconSize),
        iconTexture(stCString("menuSbsRL"),          anIconSize),
        iconTexture(stCString("menuOverUnderLR"),    anIconSize),
        iconTexture(stCString("menuOverUnderRL"),    anIconSize),
        iconTexture(stCString("menuRowLR"),          anIconSize),
        iconTexture(stCString("menuColLR"),          anIconSize),
        iconTexture(stCString("menuDual"),           anIconSize),
        iconTexture(stCString("menuFrameSeqLR"),     anIconSize),
        iconTexture(stCString("menuRedCyanLR"),      anIconSize),
        iconTexture(stCString("menuGreenMagentaLR"), anIconSize),
        iconTexture(stCString("menuYellowBlueLR"),   anIconSize),
        iconTexture(stCString("menuTiledLR"),        anIconSize)
    };
    aSrcBtn->setTexturePath(aSrcTextures, StFormat_NB);
    aSrcBtn->setDrawShadow(true);
    myBtnSrcFrmt = aSrcBtn;

    myBtnSwapLR = new StGLCheckboxTextured(myPanelUpper, myImage->params.swapLR,
                                           iconTexture(stCString("actionSwapLROff"), anIconSize),
                                           iconTexture(stCString("actionSwapLROn"),  anIconSize),
                                           (aBtnIter++) * anIconStep, 0,
                                           StGLCorner(ST_VCORNER_TOP, ST_HCORNER_LEFT));
    myBtnSwapLR->setDrawShadow(true);
    myBtnSwapLR->changeMargins() = aButtonMargins;

    StHandle<StBoolParam> aTrackedPano = new StBoolParam(false);
    myBtnPanorama = new StGLCheckboxTextured(myPanelUpper, aTrackedPano,
                                             iconTexture(stCString("actionPanoramaOff"), anIconSize),
                                             iconTexture(stCString("actionPanorama"),    anIconSize),
                                             (aBtnIter++) * anIconStep, 0,
                                             StGLCorner(ST_VCORNER_TOP, ST_HCORNER_LEFT));
    myBtnPanorama->signals.onBtnClick += stSlot(this, &StImageViewerGUI::doPanoramaCombo);
    myBtnPanorama->setDrawShadow(true);
    myBtnPanorama->changeMargins() = aButtonMargins;

    aBtnIter = 0;
    StGLTextureButton* aBtnEx = new StGLTextureButton(myPanelUpper, (aBtnIter--) * (-anIconStep), 0,
                                                      StGLCorner(ST_VCORNER_TOP, ST_HCORNER_RIGHT));
    aBtnEx->changeMargins() = aButtonMargins;
    aBtnEx->setTexturePath(iconTexture(stCString("actionOverflow"), anIconSize));
    aBtnEx->setDrawShadow(true);
    aBtnEx->signals.onBtnClick += stSlot(this, &StImageViewerGUI::doShowMobileExMenu);
}

/**
 * Create bottom toolbar
 */
void StImageViewerGUI::createMobileBottomToolbar() {
    StMarginsI aButtonMargins;
    const IconSize anIconSize = scaleIcon(32, aButtonMargins);
    const int      anIconStep = scale(56);
    aButtonMargins.extend(scale(12));

    const StMarginsI& aRootMargins = getRootMargins();
    myPanelBottom = new StGLContainer(this, aRootMargins.left, -aRootMargins.bottom, StGLCorner(ST_VCORNER_BOTTOM, ST_HCORNER_LEFT), scale(4096), scale(56));

    int aBtnIter = 0;
    myBtnPrev = new StGLTextureButton(myPanelBottom, (aBtnIter++) * anIconStep, 0);
    myBtnPrev->signals.onBtnClick += stSlot(myPlugin, &StImageViewer::doListPrev);
    myBtnPrev->setTexturePath(iconTexture(stCString("actionBack"), anIconSize));
    myBtnPrev->setDrawShadow(true);
    myBtnPrev->changeMargins() = aButtonMargins;

    myBtnNext = new StGLTextureButton(myPanelBottom, (aBtnIter++) * anIconStep, 0);
    myBtnNext->signals.onBtnClick += stSlot(myPlugin, &StImageViewer::doListNext);
    myBtnNext->setTexturePath(iconTexture(stCString("actionNext"), anIconSize));
    myBtnNext->setDrawShadow(true);
    myBtnNext->changeMargins() = aButtonMargins;

    myBtnInfo = new StGLTextureButton(myPanelBottom, (aBtnIter++) * anIconStep, 0);
    myBtnInfo->signals.onBtnClick += stSlot(myPlugin, &StImageViewer::doAboutImage);
    myBtnInfo->setTexturePath(iconTexture(stCString("actionInfo"),  anIconSize));
    myBtnInfo->setDrawShadow(true);
    myBtnInfo->changeMargins() = aButtonMargins;

    aBtnIter = 0;
    StGLTextureButton* aBtnZoomIn = new StGLTextureButton(myPanelBottom, (aBtnIter++) * (-anIconStep), 0,
                                                          StGLCorner(ST_VCORNER_TOP, ST_HCORNER_RIGHT));
    aBtnZoomIn->changeMargins() = aButtonMargins;
    aBtnZoomIn->setTexturePath(iconTexture(stCString("actionZoomIn"), anIconSize));
    aBtnZoomIn->setDrawShadow(true);
    aBtnZoomIn->setUserData(StImageViewer::Action_StereoParamsBegin + StGLImageRegion::Action_ScaleIn);
    aBtnZoomIn->signals.onBtnHold += stSlot(this, &StImageViewerGUI::doAction);

    StGLTextureButton* aBtnZoomOut = new StGLTextureButton(myPanelBottom, (aBtnIter++) * (-anIconStep), 0,
        StGLCorner(ST_VCORNER_TOP, ST_HCORNER_RIGHT));
    aBtnZoomOut->changeMargins() = aButtonMargins;
    aBtnZoomOut->setTexturePath(iconTexture(stCString("actionZoomOut"), anIconSize));
    aBtnZoomOut->setDrawShadow(true);
    aBtnZoomOut->setUserData(StImageViewer::Action_StereoParamsBegin + StGLImageRegion::Action_ScaleOut);
    aBtnZoomOut->signals.onBtnHold += stSlot(this, &StImageViewerGUI::doAction);

    myBtnList = new StGLCheckboxTextured(myPanelBottom, myPlugin->params.ToShowPlayList,
                                         iconTexture(stCString("actionVideoPlaylistOff"), anIconSize),
                                         iconTexture(stCString("actionVideoPlaylist"),    anIconSize),
                                         (aBtnIter++) * (-anIconStep), 0,
                                         StGLCorner(ST_VCORNER_TOP, ST_HCORNER_RIGHT));
    myBtnList->setDrawShadow(true);
    myBtnList->changeMargins() = aButtonMargins;
}

void StImageViewerGUI::doOpenFile(const size_t ) {
    /*if(!isMobile()) {
        myPlugin->doOpen1FileDialog();
        return;
    }*/

    StGLOpenFile* aDialog = new StGLOpenFile(this, tr(DIALOG_OPEN_FILE), tr(BUTTON_CLOSE));
    aDialog->setMimeList(myPlugin->myLoader->getMimeList());
#if defined(_WIN32)
    //
#else
    aDialog->addHotItem("/", "Root");
#endif
    aDialog->addHotItem(getResourceManager()->getFolder(StResourceManager::FolderId_SdCard));
    aDialog->addHotItem(getResourceManager()->getFolder(StResourceManager::FolderId_Downloads));
    aDialog->addHotItem(getResourceManager()->getFolder(StResourceManager::FolderId_Pictures));
    aDialog->addHotItem(getResourceManager()->getFolder(StResourceManager::FolderId_Photos));
    aDialog->signals.onFileSelected = stSlot(myPlugin, &StImageViewer::doOpen1FileFromGui);

    if(myPlugin->params.lastFolder.isEmpty()) {
        StHandle<StFileNode> aCurrFile = myPlugin->myLoader->getPlayList().getCurrentFile();
        if(!aCurrFile.isNull()) {
            myPlugin->params.lastFolder = aCurrFile->isEmpty() ? aCurrFile->getFolderPath() : aCurrFile->getValue(0)->getFolderPath();
        }
    }
    aDialog->openFolder(myPlugin->params.lastFolder);
}

void StImageViewerGUI::doShowMobileExMenu(const size_t ) {
    const int aTop = scale(56);

    StHandle<StImageInfo>&   anExtraInfo = myPlugin->myFileInfo;
    StHandle<StFileNode>     aFileNode;
    StHandle<StStereoParams> aParams;
    if(anExtraInfo.isNull()
    && !myPlugin->getCurrentFile(aFileNode, aParams, anExtraInfo)) {
        anExtraInfo.nullify();
    }

    StGLMenu*     aMenu  = new StGLMenu(this, 0, aTop, StGLMenu::MENU_VERTICAL_COMPACT, true);
    StGLMenuItem* anItem = NULL;
    aMenu->setCorner(StGLCorner(ST_VCORNER_TOP, ST_HCORNER_RIGHT));
    aMenu->setContextual(true);
    if(!anExtraInfo.isNull()) {
        anItem = aMenu->addItem(tr(BUTTON_DELETE), myPlugin->getAction(StImageViewer::Action_DeleteFile));
        anItem->setIcon(stCMenuIcon("actionDiscard"));
        anExtraInfo.nullify();
    }
    anItem = aMenu->addItem(tr(MENU_HELP_ABOUT));
    anItem->setIcon(stCMenuIcon("actionHelp"));
    anItem->signals.onItemClick += stSlot(this, &StImageViewerGUI::doAboutProgram);
    //anItem = aMenu->addItem(myPlugin->StApplication::params.ActiveDevice->getActiveValue());
    anItem = aMenu->addItem(tr(MENU_HELP_SETTINGS));
    anItem->setIcon(stCMenuIcon("actionSettings"));
    anItem->signals.onItemClick += stSlot(this, &StImageViewerGUI::doMobileSettings);
    anItem = aMenu->addItem("Slideshow", myPlugin->getAction(StImageViewer::Action_SlideShow));
    anItem->setIcon(stCMenuIcon("actionSlideShow"));
    aMenu->stglInit();
    setFocus(aMenu);
}

StImageViewerGUI::StImageViewerGUI(StImageViewer*  thePlugin,
                                   StWindow*       theWindow,
                                   StTranslations* theLangMap,
                                   const StHandle<StPlayList>&       thePlayList,
                                   const StHandle<StGLTextureQueue>& theTextureQueue)
: StGLRootWidget(thePlugin->myResMgr),
  myPlugin(thePlugin),
  myWindow(theWindow),
  myLangMap(theLangMap),
  myVisibilityTimer(true),
  //
  myImage(NULL),
  myDescr(NULL),
  myMsgStack(NULL),
  myPlayList(NULL),
  //
  myMenuRoot(NULL),
  //
  myPanelUpper(NULL),
  myPanelBottom(NULL),
  myBtnOpen(NULL),
  myBtnPrev(NULL),
  myBtnNext(NULL),
  myBtnInfo(NULL),
  myBtnSwapLR(NULL),
  myBtnPanorama(NULL),
  myBtnSrcFrmt(NULL),
  myBtnList(NULL),
  myBtnFull(NULL),
  //
  myFpsWidget(NULL),
  myHKeysTable(NULL),
  //
  myIsVisibleGUI(true),
  myIsMinimalGUI(true) {
    const GLfloat aScale = myPlugin->params.ScaleHiDPI2X->getValue() ? 2.0f : myPlugin->params.ScaleHiDPI ->getValue();
    setScale(aScale, (StGLRootWidget::ScaleAdjust )myPlugin->params.ScaleAdjust->getValue());
    setMobile(myPlugin->params.IsMobileUI->getValue());

    changeRootMargins() = myWindow->getMargins();
    myPlugin->params.ToShowFps->signals.onChanged.connect(this, &StImageViewerGUI::doShowFPS);

    StHandle<StGLTextureQueue> aTextureQueue = theTextureQueue;
    if(aTextureQueue.isNull()) {
        aTextureQueue = new StGLTextureQueue(2);
    }

    myImage = new StGLImageRegion(this, aTextureQueue, true);
    myImage->params.displayMode->setName(tr(MENU_VIEW_DISPLAY_MODE));
    myImage->params.displayMode->changeValues()[StGLImageRegion::MODE_STEREO]     = tr(MENU_VIEW_DISPLAY_MODE_STEREO);
    myImage->params.displayMode->changeValues()[StGLImageRegion::MODE_ONLY_LEFT]  = tr(MENU_VIEW_DISPLAY_MODE_LEFT);
    myImage->params.displayMode->changeValues()[StGLImageRegion::MODE_ONLY_RIGHT] = tr(MENU_VIEW_DISPLAY_MODE_RIGHT);
    myImage->params.displayMode->changeValues()[StGLImageRegion::MODE_PARALLEL]   = tr(MENU_VIEW_DISPLAY_MODE_PARALLEL);
    myImage->params.displayMode->changeValues()[StGLImageRegion::MODE_CROSSYED]   = tr(MENU_VIEW_DISPLAY_MODE_CROSSYED);
    myImage->params.ViewMode->signals.onChanged += stSlot(myPlugin, &StImageViewer::doSwitchViewMode);

    if(isMobile()) {
        createMobileUI(thePlayList);
    } else {
        createDesktopUI(thePlayList);
    }

    myMsgStack = new StGLMsgStack(this, myPlugin->getMessagesQueue());
}

StImageViewerGUI::~StImageViewerGUI() {
    //
}

bool StImageViewerGUI::toHideCursor() const {
    return (myMenuRoot != NULL) && !myMenuRoot->isVisible();
}

namespace {

    inline bool isPointIn(const StGLWidget* theWidget,
                          const StPointD_t& theCursorZo) {
        return theWidget != NULL
            && theWidget->isVisibleWithParents()
            && theWidget->isPointIn(theCursorZo);
    }

}

size_t StImageViewerGUI::trSrcFormatId(const StFormat theSrcFormat) {
    switch(theSrcFormat) {
        case StFormat_Mono:                 return MENU_SRC_FORMAT_MONO;
        case StFormat_SideBySide_LR:        return MENU_SRC_FORMAT_PARALLEL;
        case StFormat_SideBySide_RL:        return MENU_SRC_FORMAT_CROSS_EYED;
        case StFormat_TopBottom_LR:         return MENU_SRC_FORMAT_OVERUNDER_LR;
        case StFormat_TopBottom_RL:         return MENU_SRC_FORMAT_OVERUNDER_RL;
        case StFormat_Rows:                 return MENU_SRC_FORMAT_INTERLACED;
        case StFormat_AnaglyphGreenMagenta: return MENU_SRC_FORMAT_ANA_RB;
        case StFormat_AnaglyphRedCyan:      return MENU_SRC_FORMAT_ANA_RC;
        case StFormat_AnaglyphYellowBlue:   return MENU_SRC_FORMAT_ANA_YB;
        case StFormat_SeparateFrames:       return MENU_SRC_FORMAT_SEPARATE;
        default:
        case StFormat_AUTO:                 return MENU_SRC_FORMAT_AUTO;
    }
}

void StImageViewerGUI::setVisibility(const StPointD_t& theCursor,
                                     bool              toForceHide) {
    const bool toShowPlayList = myPlugin->params.ToShowPlayList->getValue();
    const bool hasMainMenu    =  myPlugin->params.ToShowMenu->getValue()
                             &&  myMenuRoot != NULL;
    const bool hasUpperPanel  = !myIsMinimalGUI
                             &&  myPlugin->params.ToShowTopbar->getValue()
                             &&  myPanelUpper  != NULL;
    const bool hasBottomPanel = !myIsMinimalGUI
                             &&  myPanelBottom != NULL;
    const bool isMouseActive  = myWindow->isMouseMoved();

    StHandle<StStereoParams> aParams = myImage->getSource();
    const double aStillTime = myVisibilityTimer.getElapsedTime();
    myIsVisibleGUI = isMouseActive
        || aParams.isNull()
        || aStillTime < 2.0
        || (hasUpperPanel  && myPanelUpper ->isPointIn(theCursor))
        || (hasBottomPanel && myPanelBottom->isPointIn(theCursor))
        || (myPlayList != NULL && toShowPlayList && myPlayList->isPointIn(theCursor))
        || (hasMainMenu    && myMenuRoot->isActive());

    if(isMouseActive) {
        myVisibilityTimer.restart();
    }
    const bool  toShowAll = !myIsMinimalGUI && myIsVisibleGUI && !toForceHide;
    const float anOpacity = (float )myVisLerp.perform(toShowAll, toForceHide);

    if(myMenuRoot != NULL) {
        myMenuRoot->setOpacity(hasMainMenu ? anOpacity : 0.0f, true);
    }
    if(myPanelUpper != NULL) {
        myPanelUpper->setOpacity(hasUpperPanel ? anOpacity : 0.0f, true);
    }
    if(myPanelBottom != NULL) {
        myPanelBottom->setOpacity(hasBottomPanel ? anOpacity : 0.0f, true);
    }
    if(myPlayList != NULL
    && toShowPlayList) {
        myPlayList->setOpacity(anOpacity, true);
    }
    if(myBtnFull != NULL) {
        if(myIsMinimalGUI
        && myPanelBottom != NULL) {
            myPanelBottom->setOpacity(1.0f, false);
        }
        myBtnFull->setOpacity(myIsMinimalGUI ? 1.0f : anOpacity, false);
    }

    const StPlayList::CurrentPosition aCurrPos = myPlugin->myLoader->getPlayList().getCurrentPosition();
    if(myBtnPrev != NULL) {
        myBtnPrev->setOpacityScale(aCurrPos == StPlayList::CurrentPosition_Middle
                                || aCurrPos == StPlayList::CurrentPosition_Last ? 1.0f : 0.5f);
    }
    if(myBtnNext != NULL) {
        myBtnNext->setOpacityScale(aCurrPos == StPlayList::CurrentPosition_Middle
                                || aCurrPos == StPlayList::CurrentPosition_First ? 1.0f : 0.5f);
    }

    StFormat aSrcFormat = (StFormat )myPlugin->params.srcFormat->getValue();
    if(aSrcFormat == StFormat_AUTO
    && !aParams.isNull()) {
        aSrcFormat = aParams->StereoFormat;
    }
    if(!aParams.isNull()
     && myImage->params.swapLR->getValue()) {
        aSrcFormat = st::formatReversed(aSrcFormat);
    }
    if(myBtnSrcFrmt != NULL) {
        myBtnSrcFrmt->setFaceId(aSrcFormat != StFormat_AUTO ? aSrcFormat : StFormat_Mono);
    }
    if(myBtnSwapLR != NULL) {
        myBtnSwapLR->setOpacity(aSrcFormat != StFormat_Mono ? myPanelUpper->getOpacity() : 0.0f, false);
    }

    const StStereoParams::ViewMode aViewMode = !aParams.isNull()
                                             ? aParams->ViewingMode
                                             : StStereoParams::FLAT_IMAGE;
    bool toShowPano = aViewMode != StStereoParams::FLAT_IMAGE;
    if(!toShowPano
    && !aParams.isNull()
    &&  st::probePanorama(aParams->StereoFormat,
                          aParams->Src1SizeX, aParams->Src1SizeY,
                          aParams->Src2SizeX, aParams->Src2SizeY) != StPanorama_OFF) {
        toShowPano = true;
    }
    if(myBtnPanorama != NULL) {
        myBtnPanorama->getTrackedValue()->setValue(aViewMode != StStereoParams::FLAT_IMAGE);
        myBtnPanorama->setOpacity(toShowPano ? myPanelUpper->getOpacity() : 0.0f, false);
    }
    myWindow->setTrackOrientation(aViewMode != StStereoParams::FLAT_IMAGE
                               && myPlugin->params.ToTrackHead->getValue());
    StQuaternion<double> aQ = myWindow->getDeviceOrientation();
    myImage->setDeviceOrientation(StGLQuaternion((float )aQ.x(), (float )aQ.y(), (float )aQ.z(), (float )aQ.w()));

    if(myDescr != NULL) {
        bool wasEmpty = myDescr->getText().isEmpty();
        if(::isPointIn(myBtnOpen, theCursor)) {
            myDescr->setText(tr(IMAGE_OPEN));
        } else if(::isPointIn(myBtnPrev, theCursor)) {
            myDescr->setText(tr(IMAGE_PREVIOUS));
        } else if(::isPointIn(myBtnNext, theCursor)) {
            myDescr->setText(tr(IMAGE_NEXT));
        } else if(::isPointIn(myBtnInfo, theCursor)) {
            myDescr->setText(tr(MENU_MEDIA_FILE_INFO));
        } else if(::isPointIn(myBtnSwapLR, theCursor)) {
            size_t aLngId = myImage->params.swapLR->getValue() ? SWAP_LR_ON : SWAP_LR_OFF;
            myDescr->setText(tr(aLngId));
        } else if(::isPointIn(myBtnList, theCursor)) {
            myDescr->setText(tr(PLAYLIST));
        } else if(::isPointIn(myBtnFull, theCursor)) {
            myDescr->setText(tr(FULLSCREEN));
        } else if(::isPointIn(myBtnSrcFrmt, theCursor)) {
            myDescr->setText(tr(BTN_SRC_FORMAT) + "\n" + trSrcFormat(aSrcFormat));
        } else if(::isPointIn(myBtnPanorama, theCursor)) {
            size_t aTrPano = MENU_VIEW_SURFACE_PLANE;
            switch(aViewMode) {
                case StStereoParams::FLAT_IMAGE:       aTrPano = MENU_VIEW_SURFACE_PLANE;   break;
                case StStereoParams::PANORAMA_SPHERE:  aTrPano = MENU_VIEW_SURFACE_SPHERE;  break;
                case StStereoParams::PANORAMA_CUBEMAP: aTrPano = MENU_VIEW_SURFACE_CUBEMAP; break;
            }
            myDescr->setText(tr(MENU_VIEW_PANORAMA) + "\n" + tr(aTrPano));
        } else {
            myDescr->setText("");
        }

        if(wasEmpty
        && aStillTime < 1.0) {
            myDescr->setText("");
        } else if(getFocus() != NULL
               || (myMenuRoot != NULL && myMenuRoot->isActive())) {
            // hide within active dialog - should be replaced by z-layer check
            myDescr->setText("");
        }

        myDescr->setOpacity(!myDescr->getText().isEmpty() ? 1.0f : 0.0f, true);
    }
}

void StImageViewerGUI::stglUpdate(const StPointD_t& pointZo) {
    StGLRootWidget::stglUpdate(pointZo);
    if(myDescr != NULL) {
        myDescr->setPoint(pointZo);
    }
    if(myLangMap->wasReloaded()) {
        myPlugin->myToRecreateMenu = true;
        myLangMap->resetReloaded();
        StImageViewerStrings::loadDefaults(*myLangMap);
    }
}

void StImageViewerGUI::stglResize(const StGLBoxPx& theRectPx) {
    const int aSizeX = theRectPx.width();
    const int aSizeY = theRectPx.height();
    myImage->changeRectPx().bottom() = aSizeY;
    myImage->changeRectPx().right()  = aSizeX;

    const StMarginsI& aMargins = myWindow->getMargins();
    const bool areNewMargins = aMargins != getRootMargins();
    if(areNewMargins) {
        changeRootMargins() = aMargins;
    }

    if(myPanelUpper != NULL) {
        myPanelUpper->changeRectPx().right() = aSizeX;
        myIsMinimalGUI = (myWindow->isMovable() || myWindow->hasFullscreenMode()) && !isMobile()
                     && (aSizeY < scale(400) || aSizeX < scale(400));
    }
    if(myPanelBottom != NULL) {
        myPanelBottom->changeRectPx().right() = aSizeX;
    }
    if(areNewMargins) {
        if(myPanelUpper != NULL) {
            myPanelUpper->changeRectPx().left() = aMargins.left;
            myPanelUpper->changeRectPx().top()  = aMargins.top;
        }
        if(myPanelBottom != NULL) {
            myPanelBottom->changeRectPx().left() = aMargins.left;
            myPanelBottom->changeRectPx().top()  = aMargins.top;
        }
        if(myMenuRoot != NULL) {
            myMenuRoot->changeRectPx().left() = aMargins.left;
            myMenuRoot->changeRectPx().top()  = aMargins.top;
            myMenuRoot->stglUpdateSubmenuLayout();
        }
    }

    StGLRootWidget::stglResize(theRectPx);
}

void StImageViewerGUI::stglDraw(unsigned int theView) {
    setLensDist(myPlugin->getMainWindow()->getLensDist());
    if((theView == ST_DRAW_LEFT || theView == ST_DRAW_MONO)
    && myFpsWidget != NULL) {
        myFpsWidget->update(myPlugin->getMainWindow()->isStereoOutput(),
                            myPlugin->getMainWindow()->getTargetFps(),
                            myPlugin->getMainWindow()->getStatistics());
    }
    StGLRootWidget::stglDraw(theView);
}

void StImageViewerGUI::doAction(const size_t theActionId,
                                const double theDuration) {
    myPlugin->invokeAction((int )theActionId, theDuration);
}

void StImageViewerGUI::doShowFPS(const bool ) {
    if(myFpsWidget != NULL) {
        delete myFpsWidget;
        myFpsWidget = NULL;
        return;
    }

    myFpsWidget = new StGLFpsLabel(this);
    myFpsWidget->stglInit();
}

void StImageViewerGUI::doAboutRenderer(const size_t ) {
    StString anAboutText = myPlugin->getMainWindow()->getRendererAbout();
    if(anAboutText.isEmpty()) {
        anAboutText = StString() + "Plugin '" + myPlugin->getMainWindow()->getRendererId() + "' doesn't provide description";
    }

    StGLMessageBox* aDialog = new StGLMessageBox(this, "", anAboutText, scale(512), scale(300));
    aDialog->addButton(tr(BUTTON_CLOSE));
    aDialog->stglInit();
}

void StImageViewerGUI::showUpdatesNotify() {
    StGLMessageBox* aDialog = new StGLMessageBox(this, "", tr(UPDATES_NOTIFY));
    aDialog->addButton(tr(BUTTON_CLOSE));
    aDialog->stglInit();
}
