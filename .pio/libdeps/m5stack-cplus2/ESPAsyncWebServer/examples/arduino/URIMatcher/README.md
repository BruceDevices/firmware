# AsyncURIMatcher Example

This example demonstrates the comprehensive URI matching capabilities of the ESPAsyncWebServer library using the `AsyncURIMatcher` class.

## Overview

The `AsyncURIMatcher` class provides flexible and powerful URL routing mechanisms that go beyond simple string matching. It supports various matching strategies that can be combined to create sophisticated routing rules.

**Important**: When using plain strings (not `AsyncURIMatcher` objects), the library uses auto-detection (`URIMatchAuto`) which analyzes the URI pattern and applies appropriate matching rules. This is **not** simple exact matching - it combines exact and folder matching by default!

## What's Demonstrated

This example includes two Arduino sketches:

1. **URIMatcher.ino** - Interactive web-based demonstration with a user-friendly homepage
2. **URIMatcherTest.ino** - Comprehensive test suite with automated shell script testing

Both sketches create a WiFi Access Point (`esp-captive`) for easy testing without network configuration.

## Auto-Detection Behavior

When you pass a plain string or `const char*` to `server.on()`, the `URIMatchAuto` flag is used, which:

1. **Empty URI**: Matches everything
2. **Ends with `*`**: Becomes prefix match (`URIMatchPrefix`)
3. **Contains `/*.ext`**: Becomes extension match (`URIMatchExtension`)
4. **Starts with `^` and ends with `$`**: Becomes regex match (if enabled)
5. **Everything else**: Becomes **both** exact and folder match (`URIMatchPrefixFolder | URIMatchExact`)

This means traditional string-based routes like `server.on("/path", handler)` will match:

- `/path` (exact match)
- `/path/` (folder with trailing slash)
- `/path/anything` (folder match)

But will **NOT** match `/path-suffix` (prefix without folder separator).

## Features Demonstrated

### 1. **Auto-Detection (Traditional Behavior)**

Demonstrates how traditional string-based routing automatically combines exact and folder matching.

**Examples in URIMatcher.ino:**

- `/auto` - Matches both `/auto` exactly AND `/auto/sub` as folder
- `/wildcard*` - Auto-detects as prefix match (due to trailing `*`)
- `/auto-images/*.png` - Auto-detects as extension match (due to `/*.ext` pattern)

**Examples in URIMatcherTest.ino:**

- `/exact` - Matches `/exact`, `/exact/`, and `/exact/sub`
- `/api/users` - Matches exact path and subpaths under `/api/users/`
- `/*.json` - Matches any `.json` file anywhere
- `/*.css` - Matches any `.css` file anywhere

### 2. **Exact Matching (Factory Method)**

Using `AsyncURIMatcher::exact()` matches only the exact URL, **NOT** subpaths.

**Key difference from auto-detection:** `AsyncURIMatcher::exact("/path")` matches **only** `/path`, while `server.on("/path", ...)` matches both `/path` and `/path/sub`.

**Examples in URIMatcher.ino:**

- `AsyncURIMatcher::exact("/exact")` - Matches only `/exact`

**Examples in URIMatcherTest.ino:**

- `AsyncURIMatcher::exact("/factory/exact")` - Matches only `/factory/exact`
- Does NOT match `/factory/exact/sub` (404 response)

### 3. **Prefix Matching**

Using `AsyncURIMatcher::prefix()` matches URLs that start with the specified pattern.

**Examples in URIMatcher.ino:**

- `AsyncURIMatcher::prefix("/service")` - Matches `/service`, `/service-test`, `/service/status`

**Examples in URIMatcherTest.ino:**

- `AsyncURIMatcher::prefix("/factory/prefix")` - Matches `/factory/prefix`, `/factory/prefix-test`, `/factory/prefix/sub`
- Traditional: `/api/*` - Matches `/api/data`, `/api/v1/posts`
- Traditional: `/files/*` - Matches `/files/document.pdf`, `/files/images/photo.jpg`

### 4. **Folder/Directory Matching**

Using `AsyncURIMatcher::dir()` matches URLs under a directory (automatically adds trailing slash).

**Important:** Directory matching requires a trailing slash in the URL - it does NOT match the directory itself.

**Examples in URIMatcher.ino:**

- `AsyncURIMatcher::dir("/admin")` - Matches `/admin/users`, `/admin/settings`
- Does NOT match `/admin` without trailing slash

**Examples in URIMatcherTest.ino:**

- `AsyncURIMatcher::dir("/factory/dir")` - Matches `/factory/dir/users`, `/factory/dir/sub/path`
- Does NOT match `/factory/dir` itself (404 response)

### 5. **Extension Matching**

Using `AsyncURIMatcher::ext()` matches files with specific extensions.

**Examples in URIMatcher.ino:**

- `AsyncURIMatcher::ext("/images/*.jpg")` - Matches `/images/photo.jpg`, `/images/sub/pic.jpg`

**Examples in URIMatcherTest.ino:**

- `AsyncURIMatcher::ext("/factory/files/*.txt")` - Matches `/factory/files/doc.txt`, `/factory/files/sub/readme.txt`
- Does NOT match `/factory/files/doc.pdf` (wrong extension)

### 6. **Case Insensitive Matching**

Using `AsyncURIMatcher::CaseInsensitive` flag matches URLs regardless of character case.

**Examples in URIMatcher.ino:**

- `AsyncURIMatcher::exact("/case", AsyncURIMatcher::CaseInsensitive)` - Matches `/case`, `/CASE`, `/CaSe`

**Examples in URIMatcherTest.ino:**

- Case insensitive exact: `/case/exact`, `/CASE/EXACT`, `/Case/Exact` all work
- Case insensitive prefix: `/case/prefix`, `/CASE/PREFIX-test`, `/Case/Prefix/sub` all work
- Case insensitive directory: `/case/dir/users`, `/CASE/DIR/admin`, `/Case/Dir/settings` all work
- Case insensitive extension: `/case/files/doc.pdf`, `/CASE/FILES/DOC.PDF`, `/Case/Files/Doc.Pdf` all work

### 7. **Regular Expression Matching**

Using `AsyncURIMatcher::regex()` for advanced pattern matching (requires `ASYNCWEBSERVER_REGEX`).

**Examples in URIMatcher.ino:**

```cpp
#ifdef ASYNCWEBSERVER_REGEX
AsyncURIMatcher::regex("^/user/([0-9]+)$")  // Matches /user/123, captures ID
#endif
```

**Examples in URIMatcherTest.ino:**

- Traditional regex: `^/user/([0-9]+)$` - Matches `/user/123`, `/user/456`
- Traditional regex: `^/blog/([0-9]{4})/([0-9]{2})/([0-9]{2})$` - Matches `/blog/2023/10/15`
- Factory regex: `AsyncURIMatcher::regex("^/factory/user/([0-9]+)$")` - Matches `/factory/user/123`
- Factory regex with multiple captures: `^/factory/blog/([0-9]{4})/([0-9]{2})/([0-9]{2})$`
- Case insensitive regex: `AsyncURIMatcher::regex("^/factory/search/(.+)$", AsyncURIMatcher::CaseInsensitive)`

### 8. **Combined Flags**

Multiple matching strategies can be combined using the `|` operator.

**Examples in URIMatcher.ino:**

- `AsyncURIMatcher::prefix("/MixedCase", AsyncURIMatcher::CaseInsensitive)` - Prefix match that's case insensitive

### 9. **Special Matchers**

**Examples in URIMatcherTest.ino:**

- `AsyncURIMatcher::all()` - Matches all requests (used with POST method as catch-all)

## Usage Patterns

### Traditional String-based Routing (Auto-Detection)

```cpp
// Auto-detection with exact + folder matching
server.on("/api", handler);          // Matches /api AND /api/anything
server.on("/login", handler);        // Matches /login AND /login/sub

// Auto-detection with prefix matching
server.on("/prefix*", handler);      // Matches /prefix, /prefix-test, /prefix/sub

// Auto-detection with extension matching
server.on("/images/*.jpg", handler); // Matches /images/pic.jpg, /images/sub/pic.jpg
```

### Explicit AsyncURIMatcher Syntax

### Explicit AsyncURIMatcher Syntax

```cpp
// Exact matching only
server.on(AsyncURIMatcher("/path", URIMatchExact), handler);

// Prefix matching only
server.on(AsyncURIMatcher("/api", URIMatchPrefix), handler);

// Combined flags
server.on(AsyncURIMatcher("/api", URIMatchPrefix | URIMatchCaseInsensitive), handler);
```

### Factory Functions

```cpp
// More readable and expressive
server.on(AsyncURIMatcher::exact("/login"), handler);
server.on(AsyncURIMatcher::prefix("/api"), handler);
server.on(AsyncURIMatcher::dir("/admin"), handler);
server.on(AsyncURIMatcher::ext("/images/*.jpg"), handler);

#ifdef ASYNCWEBSERVER_REGEX
server.on(AsyncURIMatcher::regex("^/user/([0-9]+)$"), handler);
#endif
```

## Available Flags

| Flag                      | Description                                                 |
| ------------------------- | ----------------------------------------------------------- |
| `URIMatchAuto`            | Auto-detect match type from pattern (default)               |
| `URIMatchExact`           | Exact URL match                                             |
| `URIMatchPrefix`          | Prefix match                                                |
| `URIMatchPrefixFolder`    | Folder prefix match (requires trailing /)                   |
| `URIMatchExtension`       | File extension match pattern                                |
| `URIMatchCaseInsensitive` | Case insensitive matching                                   |
| `URIMatchRegex`           | Regular expression matching (requires ASYNCWEBSERVER_REGEX) |

## Testing the Example

1. **Upload the sketch** to your ESP32/ESP8266
2. **Connect to WiFi AP**: `esp-captive` (no password required)
3. **Navigate to**: `http://192.168.4.1/`
4. **Explore the examples** by clicking the organized test links
5. **Monitor Serial output**: Open Serial Monitor to see detailed debugging information for each matched route

### Test URLs Available (All Clickable from Homepage)

**Auto-Detection Examples:**

- `http://192.168.4.1/auto` (exact + folder match)
- `http://192.168.4.1/auto/sub` (folder match - same handler!)
- `http://192.168.4.1/wildcard-test` (auto-detected prefix)
- `http://192.168.4.1/auto-images/photo.png` (auto-detected extension)

**Factory Method Examples:**

- `http://192.168.4.1/exact` (AsyncURIMatcher::exact)
- `http://192.168.4.1/service/status` (AsyncURIMatcher::prefix)
- `http://192.168.4.1/admin/users` (AsyncURIMatcher::dir)
- `http://192.168.4.1/images/photo.jpg` (AsyncURIMatcher::ext)

**Case Insensitive Examples:**

- `http://192.168.4.1/case` (lowercase)
- `http://192.168.4.1/CASE` (uppercase)
- `http://192.168.4.1/CaSe` (mixed case)

**Regex Examples (if ASYNCWEBSERVER_REGEX enabled):**

- `http://192.168.4.1/user/123` (captures numeric ID)
- `http://192.168.4.1/user/456` (captures numeric ID)

**Combined Flags Examples:**

- `http://192.168.4.1/mixedcase-test` (prefix + case insensitive)
- `http://192.168.4.1/MIXEDCASE/sub` (prefix + case insensitive)

### Console Output

Each handler provides detailed debugging information via Serial output:

```
Auto-Detection Match (Traditional)
Matched URL: /auto
Uses auto-detection: exact + folder matching
```

```
Factory Exact Match
Matched URL: /exact
Uses AsyncURIMatcher::exact() factory function
```

```
Regex Match - User ID
Matched URL: /user/123
Captured User ID: 123
This regex matches /user/{number} pattern
```

## Compilation Options

### Enable Regex Support

To enable regular expression matching, compile with:

```
-D ASYNCWEBSERVER_REGEX
```

In PlatformIO, add to `platformio.ini`:

```ini
build_flags = -D ASYNCWEBSERVER_REGEX
```

In Arduino IDE, add to your sketch:

```cpp
#define ASYNCWEBSERVER_REGEX
```

## Performance Considerations

1. **Exact matches** are fastest
2. **Prefix matches** are very efficient
3. **Regex matches** are slower but most flexible
4. **Case insensitive** matching adds minimal overhead
5. **Auto-detection** adds slight parsing overhead at construction time

## Real-World Applications

### REST API Design

```cpp
// API versioning
server.on(AsyncURIMatcher::prefix("/api/v1"), handleAPIv1);
server.on(AsyncURIMatcher::prefix("/api/v2"), handleAPIv2);

// Resource endpoints with IDs
server.on(AsyncURIMatcher::regex("^/api/users/([0-9]+)$"), handleUserById);
server.on(AsyncURIMatcher::regex("^/api/posts/([0-9]+)/comments$"), handlePostComments);
```

### File Serving

```cpp
// Serve different file types
server.on(AsyncURIMatcher::ext("/assets/*.css"), serveCSSFiles);
server.on(AsyncURIMatcher::ext("/assets/*.js"), serveJSFiles);
server.on(AsyncURIMatcher::ext("/images/*.jpg"), serveImageFiles);
```

### Admin Interface

```cpp
// Admin section with authentication
server.on(AsyncURIMatcher::dir("/admin"), handleAdminPages);
server.on(AsyncURIMatcher::exact("/admin"), redirectToAdminDashboard);
```

## See Also

- [ESPAsyncWebServer Documentation](https://github.com/ESP32Async/ESPAsyncWebServer)
- [Regular Expression Reference](https://en.cppreference.com/w/cpp/regex)
- Other examples in the `examples/` directory
