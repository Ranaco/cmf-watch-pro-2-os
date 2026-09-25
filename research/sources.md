# Pinned research sources

Research repositories are cloned outside this project and are never vendored, executed as firmware, or treated as authoritative without corroboration.

| Source | Pinned commit | Use | Trust |
| --- | --- | --- | --- |
| <https://github.com/lvgl/lv_port_actions_technology> | `26f51e584940d5b141cd1d09d2629d69b595f579` | Actions ATS30xx Zephyr/LVGL SDK and evaluation-board reference | Vendor/LVGL reference for its own EVB only |
| <https://github.com/joshuapassos/CMF-Watch-Pro-2-BLE-Protocol> | `c1e5993fffe40385bdf5e3967abaa0c25293d8f3` | Unofficial, confidence-labelled BLE research | Community evidence; never assumed correct |
| <https://github.com/whatotter/cmf-watch-firmware> | `fd8c7081c2e36355fdfc65391878fe57e528c456` | Stock image metadata and strings research | Community dump; binaries and scripts are not executed |

The Actions reference contains bootloader, recovery, radio, and filesystem blobs. Their presence is recorded but none is copied into this repository or executed. The community firmware dump contains a stock image and experimental extraction/recompilation scripts; none is executed.
