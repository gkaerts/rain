OPENUSD_DOWNLOAD_URL = "https://github.com/PixarAnimationStudios/OpenUSD/archive/refs/tags/v24.11.zip"
OPENUSD_DOWNLOAD_NAME = "OpenUSD_24_11.zip"
OPENUSD_DOWNLOAD_FULL_PATH = DOWNLOADS_PATH .. "/" .. OPENUSD_DOWNLOAD_NAME
OPENUSD_SOURCE_PATH = DOWNLOADS_PATH .. "/" .. "OpenUSD_24_11"

OPENUSD_BUILD_PATH = PROJECT_ROOT .. "/build/OpenUSD"

OPENUSD_INCLUDE_DIRS = {
    "$(RN_PYTHON_DIR)/include",
    OPENUSD_BUILD_PATH .. "/%{cfg.buildcfg}/include"
}

OPENUSD_LIB_DIRS = {
    "$(RN_PYTHON_DIR)/libs",
    OPENUSD_BUILD_PATH .. "/%{cfg.buildcfg}/lib"
}