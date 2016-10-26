#include <jni.h>
#include <string>
#include "utils/Vector.h"
#include "utils/Asset.h"
#include "utils/AssetManager.h"
#include "utils/ResourceTypes.h"
#include "utils/String8.h"

using  namespace android;

static std::string g_ReturnString;

int	 writeString(const char * s, ...)
{
    char buf[4096];
    memset(buf,0,sizeof buf);
    va_list arg_ptr ;
    va_start(arg_ptr , s);
    vsprintf(buf,s,arg_ptr);
    va_end(arg_ptr);
    g_ReturnString += buf;
    return 0;
}

static ssize_t indexOfAttribute(const ResXMLTree& tree, uint32_t attrRes)
{
    size_t N = tree.getAttributeCount();
    for (size_t i=0; i<N; i++) {
        if (tree.getAttributeNameResID(i) == attrRes) {
            return (ssize_t)i;
        }
    }
    return -1;
}

String8 getAttribute(const ResXMLTree& tree, const char* ns,
                     const char* attr, String8* outError)
{
    ssize_t idx = tree.indexOfAttribute(ns, attr);
    if (idx < 0) {
        return String8();
    }
    Res_value value;
    if (tree.getAttributeValue(idx, &value) != NO_ERROR) {
        if (value.dataType != Res_value::TYPE_STRING) {
            if (outError != NULL) *outError = "attribute is not a string value";
            return String8();
        }
    }
    size_t len;
    const uint16_t* str = tree.getAttributeStringValue(idx, &len);
    return str ? String8(str, len) : String8();
}

static String8 getAttribute(const ResXMLTree& tree, uint32_t attrRes, String8* outError)
{
    ssize_t idx = indexOfAttribute(tree, attrRes);
    if (idx < 0) {
        return String8();
    }
    Res_value value;
    if (tree.getAttributeValue(idx, &value) != NO_ERROR) {
        if (value.dataType != Res_value::TYPE_STRING) {
            if (outError != NULL) *outError = "attribute is not a string value";
            return String8();
        }
    }
    size_t len;
    const uint16_t* str = tree.getAttributeStringValue(idx, &len);
    return str ? String8(str, len) : String8();
}

static int32_t getIntegerAttribute(const ResXMLTree& tree, uint32_t attrRes,
                                   String8* outError, int32_t defValue = -1)
{
    ssize_t idx = indexOfAttribute(tree, attrRes);
    if (idx < 0) {
        return defValue;
    }
    Res_value value;
    if (tree.getAttributeValue(idx, &value) != NO_ERROR) {
        if (value.dataType < Res_value::TYPE_FIRST_INT
            || value.dataType > Res_value::TYPE_LAST_INT) {
            if (outError != NULL) *outError = "attribute is not an integer value";
            return defValue;
        }
    }
    return value.data;
}

static int32_t getResolvedIntegerAttribute(const ResTable* resTable, const ResXMLTree& tree,
                                           uint32_t attrRes, String8* outError, int32_t defValue = -1)
{
    ssize_t idx = indexOfAttribute(tree, attrRes);
    if (idx < 0) {
        return defValue;
    }
    Res_value value;
    if (tree.getAttributeValue(idx, &value) != NO_ERROR) {
        if (value.dataType == Res_value::TYPE_REFERENCE) {
            resTable->resolveReference(&value, 0);
        }
        if (value.dataType < Res_value::TYPE_FIRST_INT
            || value.dataType > Res_value::TYPE_LAST_INT) {
            if (outError != NULL) *outError = "attribute is not an integer value";
            return defValue;
        }
    }
    return value.data;
}

static String8 getResolvedAttribute(const ResTable* resTable, const ResXMLTree& tree,
                                    uint32_t attrRes, String8* outError)
{
    ssize_t idx = indexOfAttribute(tree, attrRes);
    if (idx < 0) {
        return String8();
    }
    Res_value value;
    if (tree.getAttributeValue(idx, &value) != NO_ERROR) {
        if (value.dataType == Res_value::TYPE_STRING) {
            size_t len;
            const uint16_t* str = tree.getAttributeStringValue(idx, &len);
            return str ? String8(str, len) : String8();
        }
        resTable->resolveReference(&value, 0);
        if (value.dataType != Res_value::TYPE_STRING) {
            if (outError != NULL) *outError = "attribute is not a string value";
            return String8();
        }
    }
    size_t len;
    const Res_value* value2 = &value;
    const uint16_t* str = const_cast<ResTable*>(resTable)->valueToString(value2, 0, NULL, &len);
    return str ? String8(str, len) : String8();
}

enum {
    LABEL_ATTR = 0x01010001,
    ICON_ATTR = 0x01010002,
    NAME_ATTR = 0x01010003,
    VERSION_CODE_ATTR = 0x0101021b,
    VERSION_NAME_ATTR = 0x0101021c,
    SCREEN_ORIENTATION_ATTR = 0x0101001e,
    MIN_SDK_VERSION_ATTR = 0x0101020c,
    MAX_SDK_VERSION_ATTR = 0x01010271,
    REQ_TOUCH_SCREEN_ATTR = 0x01010227,
    REQ_KEYBOARD_TYPE_ATTR = 0x01010228,
    REQ_HARD_KEYBOARD_ATTR = 0x01010229,
    REQ_NAVIGATION_ATTR = 0x0101022a,
    REQ_FIVE_WAY_NAV_ATTR = 0x01010232,
    TARGET_SDK_VERSION_ATTR = 0x01010270,
    TEST_ONLY_ATTR = 0x01010272,
    ANY_DENSITY_ATTR = 0x0101026c,
    GL_ES_VERSION_ATTR = 0x01010281,
    SMALL_SCREEN_ATTR = 0x01010284,
    NORMAL_SCREEN_ATTR = 0x01010285,
    LARGE_SCREEN_ATTR = 0x01010286,
    XLARGE_SCREEN_ATTR = 0x010102bf,
    REQUIRED_ATTR = 0x0101028e,
    SCREEN_SIZE_ATTR = 0x010102ca,
    SCREEN_DENSITY_ATTR = 0x010102cb,
    REQUIRES_SMALLEST_WIDTH_DP_ATTR = 0x01010364,
    COMPATIBLE_WIDTH_LIMIT_DP_ATTR = 0x01010365,
    LARGEST_WIDTH_LIMIT_DP_ATTR = 0x01010366,
    PUBLIC_KEY_ATTR = 0x010103a6,
};

const char *getComponentName(String8 &pkgName, String8 &componentName) {
    ssize_t idx = componentName.find(".");
    String8 retStr(pkgName);
    if (idx == 0) {
        retStr += componentName;
    } else if (idx < 0) {
        retStr += ".";
        retStr += componentName;
    } else {
        return componentName.string();
    }
    return retStr.string();
}


static void printCompatibleScreens(ResXMLTree& tree) {
    size_t len;
    ResXMLTree::event_code_t code;
    int depth = 0;
    bool first = true;
    writeString("compatible-screens:");
    while ((code=tree.next()) != ResXMLTree::END_DOCUMENT && code != ResXMLTree::BAD_DOCUMENT) {
        if (code == ResXMLTree::END_TAG) {
            depth--;
            if (depth < 0) {
                break;
            }
            continue;
        }
        if (code != ResXMLTree::START_TAG) {
            continue;
        }
        depth++;
        String8 tag(tree.getElementName(&len));
        if (tag == "screen") {
            int32_t screenSize = getIntegerAttribute(tree,
                                                     SCREEN_SIZE_ATTR, NULL, -1);
            int32_t screenDensity = getIntegerAttribute(tree,
                                                        SCREEN_DENSITY_ATTR, NULL, -1);
            if (screenSize > 0 && screenDensity > 0) {
                if (!first) {
                    writeString(",");
                }
                first = false;
                writeString("'%d/%d'", screenSize, screenDensity);
            }
        }
    }
    writeString("\n");
}






int doDump(const char * filename)
{
    int result = 0;
    Asset *asset = NULL;

    const char *option = "badging";

    AssetManager assets;
    void *assetsCookie;
    if (!assets.addAssetPath(String8(filename), &assetsCookie))
    {
        return 1;
    }

    ResTable_config config;
    config.language[0] = 'e';
    config.language[1] = 'n';
    config.country[0] = 'U';
    config.country[1] = 'S';
    config.orientation = ResTable_config::ORIENTATION_PORT;
    config.density = ResTable_config::DENSITY_MEDIUM;
    config.sdkVersion = 10000; // Very high.
    config.screenWidthDp = 320;
    config.screenHeightDp = 480;
    config.smallestScreenWidthDp = 320;
    assets.setConfiguration(config);

    const ResTable &res = assets.getResources(false);
    {
        ResXMLTree tree;
        asset = assets.openNonAsset("AndroidManifest.xml", Asset::ACCESS_BUFFER);

        if (asset == NULL)
        {
            goto bail;
        }

        if (tree.setTo(asset->getBuffer(true), asset->getLength()) != NO_ERROR)
        {
            goto bail;
        }
        tree.restart();

        if (strcmp("badging", option) == 0)
        {
            Vector <String8> locales;
            res.getLocales(&locales);

            Vector <ResTable_config> configs;
            res.getConfigurations(&configs);
            SortedVector<int> densities;
            const size_t NC = configs.size();
            for (size_t i = 0; i < NC; i++)
            {
                int dens = configs[i].density;
                if (dens == 0) dens = 160;
                densities.add(dens);
            }

            size_t len;
            ResXMLTree::event_code_t code;
            int depth = 0;
            String8 error;
            bool withinActivity = false;
            bool isMainActivity = false;
            bool isLauncherActivity = false;
            bool isSearchable = false;
            bool withinApplication = false;
            bool withinReceiver = false;
            bool withinService = false;
            bool withinIntentFilter = false;
            bool hasMainActivity = false;
            bool hasOtherActivities = false;
            bool hasOtherReceivers = false;
            bool hasOtherServices = false;
            bool hasWallpaperService = false;
            bool hasImeService = false;
            bool hasWidgetReceivers = false;
            bool hasIntentFilter = false;
            bool actMainActivity = false;
            bool actWidgetReceivers = false;
            bool actImeService = false;
            bool actWallpaperService = false;

            // This next group of variables is used to implement a group of
            // backward-compatibility heuristics necessitated by the addition of
            // some new uses-feature constants in 2.1 and 2.2. In most cases, the
            // heuristic is "if an app requests a permission but doesn't explicitly
            // request the corresponding <uses-feature>, presume it's there anyway".
            bool specCameraFeature = false; // camera-related
            bool specCameraAutofocusFeature = false;
            bool reqCameraAutofocusFeature = false;
            bool reqCameraFlashFeature = false;
            bool hasCameraPermission = false;
            bool specLocationFeature = false; // location-related
            bool specNetworkLocFeature = false;
            bool reqNetworkLocFeature = false;
            bool specGpsFeature = false;
            bool reqGpsFeature = false;
            bool hasMockLocPermission = false;
            bool hasCoarseLocPermission = false;
            bool hasGpsPermission = false;
            bool hasGeneralLocPermission = false;
            bool specBluetoothFeature = false; // Bluetooth API-related
            bool hasBluetoothPermission = false;
            bool specMicrophoneFeature = false; // microphone-related
            bool hasRecordAudioPermission = false;
            bool specWiFiFeature = false;
            bool hasWiFiPermission = false;
            bool specTelephonyFeature = false; // telephony-related
            bool reqTelephonySubFeature = false;
            bool hasTelephonyPermission = false;
            bool specTouchscreenFeature = false; // touchscreen-related
            bool specMultitouchFeature = false;
            bool reqDistinctMultitouchFeature = false;
            bool specScreenPortraitFeature = false;
            bool specScreenLandscapeFeature = false;
            bool reqScreenPortraitFeature = false;
            bool reqScreenLandscapeFeature = false;
            // 2.2 also added some other features that apps can request, but that
            // have no corresponding permission, so we cannot implement any
            // back-compatibility heuristic for them. The below are thus unnecessary
            // (but are retained here for documentary purposes.)
            //bool specCompassFeature = false;
            //bool specAccelerometerFeature = false;
            //bool specProximityFeature = false;
            //bool specAmbientLightFeature = false;
            //bool specLiveWallpaperFeature = false;

            int targetSdk = 0;
            int smallScreen = 1;
            int normalScreen = 1;
            int largeScreen = 1;
            int xlargeScreen = 1;
            int anyDensity = 1;
            int requiresSmallestWidthDp = 0;
            int compatibleWidthLimitDp = 0;
            int largestWidthLimitDp = 0;
            String8 pkg;
            String8 activityName;
            String8 activityLabel;
            String8 activityIcon;
            String8 receiverName;
            String8 serviceName;
            while ((code = tree.next()) != ResXMLTree::END_DOCUMENT &&
                   code != ResXMLTree::BAD_DOCUMENT)
            {
                if (code == ResXMLTree::END_TAG)
                {
                    depth--;
                    if (depth < 2)
                    {
                        withinApplication = false;
                    } else if (depth < 3)
                    {
                        if (withinActivity && isMainActivity && isLauncherActivity)
                        {
                            const char *aName = getComponentName(pkg, activityName);
                            writeString("launchable-activity:");
                            if (aName != NULL)
                            {
                                writeString(" name='%s' ", aName);
                            }
                            writeString(" label='%s' icon='%s'\n",
                                   activityLabel.string(),
                                   activityIcon.string());
                        }
                        if (!hasIntentFilter)
                        {
                            hasOtherActivities |= withinActivity;
                            hasOtherReceivers |= withinReceiver;
                            hasOtherServices |= withinService;
                        }
                        withinActivity = false;
                        withinService = false;
                        withinReceiver = false;
                        hasIntentFilter = false;
                        isMainActivity = isLauncherActivity = false;
                    } else if (depth < 4)
                    {
                        if (withinIntentFilter)
                        {
                            if (withinActivity)
                            {
                                hasMainActivity |= actMainActivity;
                                hasOtherActivities |= !actMainActivity;
                            } else if (withinReceiver)
                            {
                                hasWidgetReceivers |= actWidgetReceivers;
                                hasOtherReceivers |= !actWidgetReceivers;
                            } else if (withinService)
                            {
                                hasImeService |= actImeService;
                                hasWallpaperService |= actWallpaperService;
                                hasOtherServices |= (!actImeService && !actWallpaperService);
                            }
                        }
                        withinIntentFilter = false;
                    }
                    continue;
                }
                if (code != ResXMLTree::START_TAG)
                {
                    continue;
                }
                depth++;
                String8 tag(tree.getElementName(&len));
                if (depth == 1)
                {
                    if (tag != "manifest")
                    {
                        goto bail;
                    }
                    pkg = getAttribute(tree, NULL, "package", NULL);
                    writeString("package: name='%s' ", pkg.string());
                    int32_t versionCode = getIntegerAttribute(tree, VERSION_CODE_ATTR, &error);
                    if (error != "")
                    {
                        goto bail;
                    }
                    if (versionCode > 0)
                    {
                        writeString("versionCode='%d' ", versionCode);
                    } else
                    {
                        writeString("versionCode='' ");
                    }
                    String8 versionName = getResolvedAttribute(&res, tree, VERSION_NAME_ATTR,
                                                               &error);
                    if (error != "")
                    {
                        goto bail;
                    }
                    writeString("versionName='%s'\n", versionName.string());
                } else if (depth == 2)
                {
                    withinApplication = false;
                    if (tag == "application")
                    {
                        withinApplication = true;

                        String8 label;
                        const size_t NL = locales.size();
                        for (size_t i = 0; i < NL; i++)
                        {
                            const char *localeStr = locales[i].string();
                            assets.setLocale(localeStr != NULL ? localeStr : "");
                            String8 llabel = getResolvedAttribute(&res, tree, LABEL_ATTR, &error);
                            if (llabel != "")
                            {
                                if (localeStr == NULL || strlen(localeStr) == 0)
                                {
                                    label = llabel;
                                    writeString("application-label:'%s'\n", llabel.string());
                                } else
                                {
                                    if (label == "")
                                    {
                                        label = llabel;
                                    }
                                    writeString("application-label-%s:'%s'\n", localeStr,
                                           llabel.string());
                                }
                            }
                        }

                        ResTable_config tmpConfig = config;
                        const size_t ND = densities.size();
                        for (size_t i = 0; i < ND; i++)
                        {
                            tmpConfig.density = densities[i];
                            assets.setConfiguration(tmpConfig);
                            String8 icon = getResolvedAttribute(&res, tree, ICON_ATTR, &error);
                            if (icon != "")
                            {
                                writeString("application-icon-%d:'%s'\n", densities[i], icon.string());
                            }
                        }
                        assets.setConfiguration(config);

                        String8 icon = getResolvedAttribute(&res, tree, ICON_ATTR, &error);
                        if (error != "")
                        {
                            goto bail;
                        }
                        int32_t testOnly = getIntegerAttribute(tree, TEST_ONLY_ATTR, &error, 0);
                        if (error != "")
                        {
                            goto bail;
                        }
                        writeString("application: label='%s' ", label.string());
                        writeString("icon='%s'\n", icon.string());
                        if (testOnly != 0)
                        {
                            writeString("testOnly='%d'\n", testOnly);
                        }
                    } else if (tag == "uses-sdk")
                    {
                        int32_t code = getIntegerAttribute(tree, MIN_SDK_VERSION_ATTR, &error);
                        if (error != "")
                        {
                            error = "";
                            String8 name = getResolvedAttribute(&res, tree, MIN_SDK_VERSION_ATTR,
                                                                &error);
                            if (error != "")
                            {
                                goto bail;
                            }
                            if (name == "Donut") targetSdk = 4;
                            writeString("sdkVersion:'%s'\n", name.string());
                        } else if (code != -1)
                        {
                            targetSdk = code;
                            writeString("sdkVersion:'%d'\n", code);
                        }
                        code = getIntegerAttribute(tree, MAX_SDK_VERSION_ATTR, NULL, -1);
                        if (code != -1)
                        {
                            writeString("maxSdkVersion:'%d'\n", code);
                        }
                        code = getIntegerAttribute(tree, TARGET_SDK_VERSION_ATTR, &error);
                        if (error != "")
                        {
                            error = "";
                            String8 name = getResolvedAttribute(&res, tree, TARGET_SDK_VERSION_ATTR,
                                                                &error);
                            if (error != "")
                            {
                                goto bail;
                            }
                            if (name == "Donut" && targetSdk < 4) targetSdk = 4;
                            writeString("targetSdkVersion:'%s'\n", name.string());
                        } else if (code != -1)
                        {
                            if (targetSdk < code)
                            {
                                targetSdk = code;
                            }
                            writeString("targetSdkVersion:'%d'\n", code);
                        }
                    } else if (tag == "uses-configuration")
                    {
                        int32_t reqTouchScreen = getIntegerAttribute(tree,
                                                                     REQ_TOUCH_SCREEN_ATTR, NULL,
                                                                     0);
                        int32_t reqKeyboardType = getIntegerAttribute(tree,
                                                                      REQ_KEYBOARD_TYPE_ATTR, NULL,
                                                                      0);
                        int32_t reqHardKeyboard = getIntegerAttribute(tree,
                                                                      REQ_HARD_KEYBOARD_ATTR, NULL,
                                                                      0);
                        int32_t reqNavigation = getIntegerAttribute(tree,
                                                                    REQ_NAVIGATION_ATTR, NULL, 0);
                        int32_t reqFiveWayNav = getIntegerAttribute(tree,
                                                                    REQ_FIVE_WAY_NAV_ATTR, NULL, 0);
                        writeString("uses-configuration:");
                        if (reqTouchScreen != 0)
                        {
                            writeString(" reqTouchScreen='%d'", reqTouchScreen);
                        }
                        if (reqKeyboardType != 0)
                        {
                            writeString(" reqKeyboardType='%d'", reqKeyboardType);
                        }
                        if (reqHardKeyboard != 0)
                        {
                            writeString(" reqHardKeyboard='%d'", reqHardKeyboard);
                        }
                        if (reqNavigation != 0)
                        {
                            writeString(" reqNavigation='%d'", reqNavigation);
                        }
                        if (reqFiveWayNav != 0)
                        {
                            writeString(" reqFiveWayNav='%d'", reqFiveWayNav);
                        }
                        writeString("\n");
                    } else if (tag == "supports-screens")
                    {
                        smallScreen = getIntegerAttribute(tree,
                                                          SMALL_SCREEN_ATTR, NULL, 1);
                        normalScreen = getIntegerAttribute(tree,
                                                           NORMAL_SCREEN_ATTR, NULL, 1);
                        largeScreen = getIntegerAttribute(tree,
                                                          LARGE_SCREEN_ATTR, NULL, 1);
                        xlargeScreen = getIntegerAttribute(tree,
                                                           XLARGE_SCREEN_ATTR, NULL, 1);
                        anyDensity = getIntegerAttribute(tree,
                                                         ANY_DENSITY_ATTR, NULL, 1);
                        requiresSmallestWidthDp = getIntegerAttribute(tree,
                                                                      REQUIRES_SMALLEST_WIDTH_DP_ATTR,
                                                                      NULL, 0);
                        compatibleWidthLimitDp = getIntegerAttribute(tree,
                                                                     COMPATIBLE_WIDTH_LIMIT_DP_ATTR,
                                                                     NULL, 0);
                        largestWidthLimitDp = getIntegerAttribute(tree,
                                                                  LARGEST_WIDTH_LIMIT_DP_ATTR, NULL,
                                                                  0);
                    } else if (tag == "uses-feature")
                    {
                        String8 name = getAttribute(tree, NAME_ATTR, &error);

                        if (name != "" && error == "")
                        {
                            int req = getIntegerAttribute(tree,
                                                          REQUIRED_ATTR, NULL, 1);

                            if (name == "android.hardware.camera")
                            {
                                specCameraFeature = true;
                            } else if (name == "android.hardware.camera.autofocus")
                            {
                                // these have no corresponding permission to check for,
                                // but should imply the foundational camera permission
                                reqCameraAutofocusFeature = reqCameraAutofocusFeature || req;
                                specCameraAutofocusFeature = true;
                            } else if (req && (name == "android.hardware.camera.flash"))
                            {
                                // these have no corresponding permission to check for,
                                // but should imply the foundational camera permission
                                reqCameraFlashFeature = true;
                            } else if (name == "android.hardware.location")
                            {
                                specLocationFeature = true;
                            } else if (name == "android.hardware.location.network")
                            {
                                specNetworkLocFeature = true;
                                reqNetworkLocFeature = reqNetworkLocFeature || req;
                            } else if (name == "android.hardware.location.gps")
                            {
                                specGpsFeature = true;
                                reqGpsFeature = reqGpsFeature || req;
                            } else if (name == "android.hardware.bluetooth")
                            {
                                specBluetoothFeature = true;
                            } else if (name == "android.hardware.touchscreen")
                            {
                                specTouchscreenFeature = true;
                            } else if (name == "android.hardware.touchscreen.multitouch")
                            {
                                specMultitouchFeature = true;
                            } else if (name == "android.hardware.touchscreen.multitouch.distinct")
                            {
                                reqDistinctMultitouchFeature = reqDistinctMultitouchFeature || req;
                            } else if (name == "android.hardware.microphone")
                            {
                                specMicrophoneFeature = true;
                            } else if (name == "android.hardware.wifi")
                            {
                                specWiFiFeature = true;
                            } else if (name == "android.hardware.telephony")
                            {
                                specTelephonyFeature = true;
                            } else if (req && (name == "android.hardware.telephony.gsm" ||
                                               name == "android.hardware.telephony.cdma"))
                            {
                                // these have no corresponding permission to check for,
                                // but should imply the foundational telephony permission
                                reqTelephonySubFeature = true;
                            } else if (name == "android.hardware.screen.portrait")
                            {
                                specScreenPortraitFeature = true;
                            } else if (name == "android.hardware.screen.landscape")
                            {
                                specScreenLandscapeFeature = true;
                            }
                            writeString("uses-feature%s:'%s'\n",
                                   req ? "" : "-not-required", name.string());
                        } else
                        {
                            int vers = getIntegerAttribute(tree,
                                                           GL_ES_VERSION_ATTR, &error);
                            if (error == "")
                            {
                                writeString("uses-gl-es:'0x%x'\n", vers);
                            }
                        }
                    } else if (tag == "uses-permission")
                    {
                        String8 name = getAttribute(tree, NAME_ATTR, &error);
                        if (name != "" && error == "")
                        {
                            if (name == "android.permission.CAMERA")
                            {
                                hasCameraPermission = true;
                            } else if (name == "android.permission.ACCESS_FINE_LOCATION")
                            {
                                hasGpsPermission = true;
                            } else if (name == "android.permission.ACCESS_MOCK_LOCATION")
                            {
                                hasMockLocPermission = true;
                            } else if (name == "android.permission.ACCESS_COARSE_LOCATION")
                            {
                                hasCoarseLocPermission = true;
                            } else if (
                                    name == "android.permission.ACCESS_LOCATION_EXTRA_COMMANDS" ||
                                    name == "android.permission.INSTALL_LOCATION_PROVIDER")
                            {
                                hasGeneralLocPermission = true;
                            } else if (name == "android.permission.BLUETOOTH" ||
                                       name == "android.permission.BLUETOOTH_ADMIN")
                            {
                                hasBluetoothPermission = true;
                            } else if (name == "android.permission.RECORD_AUDIO")
                            {
                                hasRecordAudioPermission = true;
                            } else if (name == "android.permission.ACCESS_WIFI_STATE" ||
                                       name == "android.permission.CHANGE_WIFI_STATE" ||
                                       name == "android.permission.CHANGE_WIFI_MULTICAST_STATE")
                            {
                                hasWiFiPermission = true;
                            } else if (name == "android.permission.CALL_PHONE" ||
                                       name == "android.permission.CALL_PRIVILEGED" ||
                                       name == "android.permission.MODIFY_PHONE_STATE" ||
                                       name == "android.permission.PROCESS_OUTGOING_CALLS" ||
                                       name == "android.permission.READ_SMS" ||
                                       name == "android.permission.RECEIVE_SMS" ||
                                       name == "android.permission.RECEIVE_MMS" ||
                                       name == "android.permission.RECEIVE_WAP_PUSH" ||
                                       name == "android.permission.SEND_SMS" ||
                                       name == "android.permission.WRITE_APN_SETTINGS" ||
                                       name == "android.permission.WRITE_SMS")
                            {
                                hasTelephonyPermission = true;
                            }
                            writeString("uses-permission:'%s'\n", name.string());
                        } else
                        {
                            goto bail;
                        }
                    } else if (tag == "uses-package")
                    {
                        String8 name = getAttribute(tree, NAME_ATTR, &error);
                        if (name != "" && error == "")
                        {
                            writeString("uses-package:'%s'\n", name.string());
                        } else
                        {
                            goto bail;
                        }
                    } else if (tag == "original-package")
                    {
                        String8 name = getAttribute(tree, NAME_ATTR, &error);
                        if (name != "" && error == "")
                        {
                            writeString("original-package:'%s'\n", name.string());
                        } else
                        {
                            goto bail;
                        }
                    } else if (tag == "supports-gl-texture")
                    {
                        String8 name = getAttribute(tree, NAME_ATTR, &error);
                        if (name != "" && error == "")
                        {
                            writeString("supports-gl-texture:'%s'\n", name.string());
                        } else
                        {
                            goto bail;
                        }
                    } else if (tag == "compatible-screens")
                    {
                        printCompatibleScreens(tree);
                        depth--;
                    } else if (tag == "package-verifier")
                    {
                        String8 name = getAttribute(tree, NAME_ATTR, &error);
                        if (name != "" && error == "")
                        {
                            String8 publicKey = getAttribute(tree, PUBLIC_KEY_ATTR, &error);
                            if (publicKey != "" && error == "")
                            {
                                writeString("package-verifier: name='%s' publicKey='%s'\n",
                                       name.string(), publicKey.string());
                            }
                        }
                    }
                } else if (depth == 3 && withinApplication)
                {
                    withinActivity = false;
                    withinReceiver = false;
                    withinService = false;
                    hasIntentFilter = false;
                    if (tag == "activity")
                    {
                        withinActivity = true;
                        activityName = getAttribute(tree, NAME_ATTR, &error);
                        if (error != "")
                        {
                            goto bail;
                        }

                        activityLabel = getResolvedAttribute(&res, tree, LABEL_ATTR, &error);
                        if (error != "")
                        {
                            goto bail;
                        }

                        activityIcon = getResolvedAttribute(&res, tree, ICON_ATTR, &error);
                        if (error != "")
                        {
                            goto bail;
                        }

                        int32_t orien = getResolvedIntegerAttribute(&res, tree,
                                                                    SCREEN_ORIENTATION_ATTR,
                                                                    &error);
                        if (error == "")
                        {
                            if (orien == 0 || orien == 6 || orien == 8)
                            {
                                // Requests landscape, sensorLandscape, or reverseLandscape.
                                reqScreenLandscapeFeature = true;
                            } else if (orien == 1 || orien == 7 || orien == 9)
                            {
                                // Requests portrait, sensorPortrait, or reversePortrait.
                                reqScreenPortraitFeature = true;
                            }
                        }
                    } else if (tag == "uses-library")
                    {
                        String8 libraryName = getAttribute(tree, NAME_ATTR, &error);
                        if (error != "")
                        {
                            goto bail;
                        }
                        int req = getIntegerAttribute(tree,
                                                      REQUIRED_ATTR, NULL, 1);
                        writeString("uses-library%s:'%s'\n",
                               req ? "" : "-not-required", libraryName.string());
                    } else if (tag == "receiver")
                    {
                        withinReceiver = true;
                        receiverName = getAttribute(tree, NAME_ATTR, &error);

                        if (error != "")
                        {
                            goto bail;
                        }
                    } else if (tag == "service")
                    {
                        withinService = true;
                        serviceName = getAttribute(tree, NAME_ATTR, &error);

                        if (error != "")
                        {
                            goto bail;
                        }
                    }
                } else if ((depth == 4) && (tag == "intent-filter"))
                {
                    hasIntentFilter = true;
                    withinIntentFilter = true;
                    actMainActivity = actWidgetReceivers = actImeService = actWallpaperService = false;
                } else if ((depth == 5) && withinIntentFilter)
                {
                    String8 action;
                    if (tag == "action")
                    {
                        action = getAttribute(tree, NAME_ATTR, &error);
                        if (error != "")
                        {
                            goto bail;
                        }
                        if (withinActivity)
                        {
                            if (action == "android.intent.action.MAIN")
                            {
                                isMainActivity = true;
                                actMainActivity = true;
                            }
                        } else if (withinReceiver)
                        {
                            if (action == "android.appwidget.action.APPWIDGET_UPDATE")
                            {
                                actWidgetReceivers = true;
                            }
                        } else if (withinService)
                        {
                            if (action == "android.view.InputMethod")
                            {
                                actImeService = true;
                            } else if (action == "android.service.wallpaper.WallpaperService")
                            {
                                actWallpaperService = true;
                            }
                        }
                        if (action == "android.intent.action.SEARCH")
                        {
                            isSearchable = true;
                        }
                    }

                    if (tag == "category")
                    {
                        String8 category = getAttribute(tree, NAME_ATTR, &error);
                        if (error != "")
                        {
                            goto bail;
                        }
                        if (withinActivity)
                        {
                            if (category == "android.intent.category.LAUNCHER")
                            {
                                isLauncherActivity = true;
                            }
                        }
                    }
                }
            }

            /* The following blocks handle printing "inferred" uses-features, based
             * on whether related features or permissions are used by the app.
             * Note that the various spec*Feature variables denote whether the
             * relevant tag was *present* in the AndroidManfest, not that it was
             * present and set to true.
             */
            // Camera-related back-compatibility logic
            if (!specCameraFeature)
            {
                if (reqCameraFlashFeature || reqCameraAutofocusFeature)
                {
                    // if app requested a sub-feature (autofocus or flash) and didn't
                    // request the base camera feature, we infer that it meant to
                    writeString("uses-feature:'android.hardware.camera'\n");
                } else if (hasCameraPermission)
                {
                    // if app wants to use camera but didn't request the feature, we infer
                    // that it meant to, and further that it wants autofocus
                    // (which was the 1.0 - 1.5 behavior)
                    writeString("uses-feature:'android.hardware.camera'\n");
                    if (!specCameraAutofocusFeature)
                    {
                        writeString("uses-feature:'android.hardware.camera.autofocus'\n");
                    }
                }
            }

            // Location-related back-compatibility logic
            if (!specLocationFeature &&
                (hasMockLocPermission || hasCoarseLocPermission || hasGpsPermission ||
                 hasGeneralLocPermission || reqNetworkLocFeature || reqGpsFeature))
            {
                // if app either takes a location-related permission or requests one of the
                // sub-features, we infer that it also meant to request the base location feature
                writeString("uses-feature:'android.hardware.location'\n");
            }
            if (!specGpsFeature && hasGpsPermission)
            {
                // if app takes GPS (FINE location) perm but does not request the GPS
                // feature, we infer that it meant to
                writeString("uses-feature:'android.hardware.location.gps'\n");
            }
            if (!specNetworkLocFeature && hasCoarseLocPermission)
            {
                // if app takes Network location (COARSE location) perm but does not request the
                // network location feature, we infer that it meant to
                writeString("uses-feature:'android.hardware.location.network'\n");
            }

            // Bluetooth-related compatibility logic
            if (!specBluetoothFeature && hasBluetoothPermission && (targetSdk > 4))
            {
                // if app takes a Bluetooth permission but does not request the Bluetooth
                // feature, we infer that it meant to
                writeString("uses-feature:'android.hardware.bluetooth'\n");
            }

            // Microphone-related compatibility logic
            if (!specMicrophoneFeature && hasRecordAudioPermission)
            {
                // if app takes the record-audio permission but does not request the microphone
                // feature, we infer that it meant to
                writeString("uses-feature:'android.hardware.microphone'\n");
            }

            // WiFi-related compatibility logic
            if (!specWiFiFeature && hasWiFiPermission)
            {
                // if app takes one of the WiFi permissions but does not request the WiFi
                // feature, we infer that it meant to
                writeString("uses-feature:'android.hardware.wifi'\n");
            }

            // Telephony-related compatibility logic
            if (!specTelephonyFeature && (hasTelephonyPermission || reqTelephonySubFeature))
            {
                // if app takes one of the telephony permissions or requests a sub-feature but
                // does not request the base telephony feature, we infer that it meant to
                writeString("uses-feature:'android.hardware.telephony'\n");
            }

            // Touchscreen-related back-compatibility logic
            if (!specTouchscreenFeature)
            { // not a typo!
                // all apps are presumed to require a touchscreen, unless they explicitly say
                // <uses-feature android:name="android.hardware.touchscreen" android:required="false"/>
                // Note that specTouchscreenFeature is true if the tag is present, regardless
                // of whether its value is true or false, so this is safe
                writeString("uses-feature:'android.hardware.touchscreen'\n");
            }
            if (!specMultitouchFeature && reqDistinctMultitouchFeature)
            {
                // if app takes one of the telephony permissions or requests a sub-feature but
                // does not request the base telephony feature, we infer that it meant to
                writeString("uses-feature:'android.hardware.touchscreen.multitouch'\n");
            }

            // Landscape/portrait-related compatibility logic
            if (!specScreenLandscapeFeature && !specScreenPortraitFeature)
            {
                // If the app has specified any activities in its manifest
                // that request a specific orientation, then assume that
                // orientation is required.
                if (reqScreenLandscapeFeature)
                {
                    writeString("uses-feature:'android.hardware.screen.landscape'\n");
                }
                if (reqScreenPortraitFeature)
                {
                    writeString("uses-feature:'android.hardware.screen.portrait'\n");
                }
            }

            if (hasMainActivity)
            {
                writeString("main\n");
            }
            if (hasWidgetReceivers)
            {
                writeString("app-widget\n");
            }
            if (hasImeService)
            {
                writeString("ime\n");
            }
            if (hasWallpaperService)
            {
                writeString("wallpaper\n");
            }
            if (hasOtherActivities)
            {
                writeString("other-activities\n");
            }
            if (isSearchable)
            {
                writeString("search\n");
            }
            if (hasOtherReceivers)
            {
                writeString("other-receivers\n");
            }
            if (hasOtherServices)
            {
                writeString("other-services\n");
            }

            // For modern apps, if screen size buckets haven't been specified
            // but the new width ranges have, then infer the buckets from them.
            if (smallScreen > 0 && normalScreen > 0 && largeScreen > 0 && xlargeScreen > 0
                && requiresSmallestWidthDp > 0)
            {
                int compatWidth = compatibleWidthLimitDp;
                if (compatWidth <= 0) compatWidth = requiresSmallestWidthDp;
                if (requiresSmallestWidthDp <= 240 && compatWidth >= 240)
                {
                    smallScreen = -1;
                } else
                {
                    smallScreen = 0;
                }
                if (requiresSmallestWidthDp <= 320 && compatWidth >= 320)
                {
                    normalScreen = -1;
                } else
                {
                    normalScreen = 0;
                }
                if (requiresSmallestWidthDp <= 480 && compatWidth >= 480)
                {
                    largeScreen = -1;
                } else
                {
                    largeScreen = 0;
                }
                if (requiresSmallestWidthDp <= 720 && compatWidth >= 720)
                {
                    xlargeScreen = -1;
                } else
                {
                    xlargeScreen = 0;
                }
            }

            // Determine default values for any unspecified screen sizes,
            // based on the target SDK of the package.  As of 4 (donut)
            // the screen size support was introduced, so all default to
            // enabled.
            if (smallScreen > 0)
            {
                smallScreen = targetSdk >= 4 ? -1 : 0;
            }
            if (normalScreen > 0)
            {
                normalScreen = -1;
            }
            if (largeScreen > 0)
            {
                largeScreen = targetSdk >= 4 ? -1 : 0;
            }
            if (xlargeScreen > 0)
            {
                // Introduced in Gingerbread.
                xlargeScreen = targetSdk >= 9 ? -1 : 0;
            }
            if (anyDensity > 0)
            {
                anyDensity = (targetSdk >= 4 || requiresSmallestWidthDp > 0
                              || compatibleWidthLimitDp > 0) ? -1 : 0;
            }
            writeString("supports-screens:");
            if (smallScreen != 0) writeString(" 'small'");
            if (normalScreen != 0) writeString(" 'normal'");
            if (largeScreen != 0) writeString(" 'large'");
            if (xlargeScreen != 0) writeString(" 'xlarge'");
            writeString("\n");
            writeString("supports-any-density: '%s'\n", anyDensity ? "true" : "false");
            if (requiresSmallestWidthDp > 0)
            {
                writeString("requires-smallest-width:'%d'\n", requiresSmallestWidthDp);
            }
            if (compatibleWidthLimitDp > 0)
            {
                writeString("compatible-width-limit:'%d'\n", compatibleWidthLimitDp);
            }
            if (largestWidthLimitDp > 0)
            {
                writeString("largest-width-limit:'%d'\n", largestWidthLimitDp);
            }

            writeString("locales:");
            const size_t NL = locales.size();
            for (size_t i = 0; i < NL; i++)
            {
                const char *localeStr = locales[i].string();
                if (localeStr == NULL || strlen(localeStr) == 0)
                {
                    localeStr = "--_--";
                }
                writeString(" '%s'", localeStr);
            }
            writeString("\n");

            writeString("densities:");
            const size_t ND = densities.size();
            for (size_t i = 0; i < ND; i++)
            {
                writeString(" '%d'", densities[i]);
            }
            writeString("\n");

            AssetDir *dir = assets.openNonAssetDir(assetsCookie, "lib");
            if (dir != NULL)
            {
                if (dir->getFileCount() > 0)
                {
                    writeString("native-code:");
                    for (size_t i = 0; i < dir->getFileCount(); i++)
                    {
                        writeString(" '%s'", dir->getFileName(i).string());
                    }
                    writeString("\n");
                }
                delete dir;
            }
        }
    }

    result = 1;

    bail:
    if (asset)
    {
        delete asset;
    }
    return result;
}


extern "C"
jstring
Java_com_kappa_aapt_MainActivity_getApkInfo(
        JNIEnv *env,
        jobject obj,
        jstring path)
{
    const char * csPath = env->GetStringUTFChars(path,0);
    g_ReturnString.clear();
    doDump(csPath);
    env->ReleaseStringUTFChars(path,csPath);
    return env->NewStringUTF(g_ReturnString.c_str());
}
