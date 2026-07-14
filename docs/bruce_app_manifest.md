# BAM (Bruce App Manifest)

Every Bruce app **must** have an `application.bam` file in its root directory. This file describes the app to the build system and to the firmware's app loader.

The manifest is a **JSON** file. It is the Bruce equivalent of Flipper Zero's `application.fam`.

---

## Manifest Format

```json
{
    "appid": "wifi_scanner",
    "name": "WiFi Scanner Pro",
    "version": "2.1",
    "author": "Phat K.",
    "entry_point": "app_main",
    "assets_dir": "assets"
}
```

## Field Reference

| Field | Type | Required | Max Length | Description |
|-------|------|----------|-----------|-------------|
| `appid` | string | **Yes** | — | Unique application identifier. Used as the output filename (`<appid>.bruce`). Must contain only lowercase letters, numbers, and underscores. Examples: `wifi_scanner`, `ir_remote`, `ble_spam` |
| `name` | string | **Yes** | 31 chars | Human-readable display name. This is what the user sees in the Apps menu on the device. |
| `version` | string | **Yes** | 7 chars | App version string. Examples: `"1.0"`, `"2.3.1"` |
| `author` | string | **Yes** | — | Author name or handle. |
| `entry_point` | string | **Yes** | — | Name of the C function to call when the app starts. **Must be `"app_main"`**. |
| `assets_dir` | string | No | — | Subdirectory containing image assets. Default: `"assets"`. Set to an empty string or omit if your app has no assets. |

## Validation

BBT validates the manifest on every build. If any required field is missing, the build will fail with a clear error message:

```
[bbt] ERROR: Manifest missing required field: 'author'
```

## Comparison with Flipper Zero FAM

| Feature | Flipper FAM (`application.fam`) | Bruce BAM (`application.bam`) |
|---------|--------------------------------|-------------------------------|
| Format | Python dict | JSON |
| App type field | `apptype = FlipperAppType.EXTERNAL` | Not needed (all BAPs are external) |
| Entry point | `entry_point = "app"` | `entry_point = "app_main"` |
| Icon assets | `fap_icon_assets = "images"` | `assets_dir = "assets"` |
| App icon (menu) | `fap_icon = "icon.png"` | Embedded in app name display |
| Category | `fap_category = "Tools"` | Not yet supported |
| SDK version | `fap_version = (1, 0)` | Via `BRUCE_API_VERSION` in header |
| Dependencies | `requires = ["gui"]` | Not applicable (single API struct) |

## Example: Minimal Manifest

```json
{
    "appid": "hello",
    "name": "Hello World",
    "version": "1.0",
    "author": "Developer",
    "entry_point": "app_main"
}
```

## Example: Full Manifest with Assets

```json
{
    "appid": "pixel_game",
    "name": "Pixel Adventure",
    "version": "1.2.0",
    "author": "GameDev Studio",
    "entry_point": "app_main",
    "assets_dir": "sprites"
}
```

This tells BBT to look for images in the `sprites/` subfolder instead of the default `assets/`.
