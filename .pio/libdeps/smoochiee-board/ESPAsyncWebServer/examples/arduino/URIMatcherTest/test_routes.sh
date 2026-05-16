#!/bin/bash

# URI Matcher Test Script
# Tests all routes defined in URIMatcherTest.ino

SERVER_IP="${1:-192.168.4.1}"
SERVER_PORT="80"
BASE_URL="http://${SERVER_IP}:${SERVER_PORT}"

echo "Testing URI Matcher at $BASE_URL"
echo "=================================="

# Function to test a route
test_route() {
    local path="$1"
    local expected_status="$2"
    local description="$3"

    echo -n "Testing $path ... "

    response=$(curl -s -w "HTTPSTATUS:%{http_code}" "$BASE_URL$path" 2>/dev/null)
    status_code=$(echo "$response" | grep -o "HTTPSTATUS:[0-9]*" | cut -d: -f2)

    if [ "$status_code" = "$expected_status" ]; then
        echo "✅ PASS ($status_code)"
    else
        echo "❌ FAIL (expected $expected_status, got $status_code)"
        return 1
    fi
    return 0
}

# Test counter
PASS=0
FAIL=0

# Test all routes that should return 200 OK
echo "Testing routes that should work (200 OK):"

if test_route "/status" "200" "Status endpoint"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/exact" "200" "Exact path"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/exact/" "200" "Exact path ending with /"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/exact/sub" "200" "Exact path with subpath /"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/api/users" "200" "Exact API path"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/api/data" "200" "API prefix match"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/api/v1/posts" "200" "API prefix deep"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/files/document.pdf" "200" "Files prefix"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/files/images/photo.jpg" "200" "Files prefix deep"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/config.json" "200" "JSON extension"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/data/settings.json" "200" "JSON extension in subfolder"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/style.css" "200" "CSS extension"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/assets/main.css" "200" "CSS extension in subfolder"; then ((PASS++)); else ((FAIL++)); fi

echo ""
echo "Testing AsyncURIMatcher factory methods:"

# Factory exact match (should NOT match subpaths)
if test_route "/factory/exact" "200" "Factory exact match"; then ((PASS++)); else ((FAIL++)); fi

# Factory prefix match
if test_route "/factory/prefix" "200" "Factory prefix base"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/factory/prefix-test" "200" "Factory prefix extended"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/factory/prefix/sub" "200" "Factory prefix subpath"; then ((PASS++)); else ((FAIL++)); fi

# Factory directory match (should NOT match the directory itself)
if test_route "/factory/dir/users" "200" "Factory directory match"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/factory/dir/sub/path" "200" "Factory directory deep"; then ((PASS++)); else ((FAIL++)); fi

# Factory extension match
if test_route "/factory/files/doc.txt" "200" "Factory extension match"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/factory/files/sub/readme.txt" "200" "Factory extension deep"; then ((PASS++)); else ((FAIL++)); fi

echo ""
echo "Testing case insensitive matching:"

# Case insensitive exact
if test_route "/case/exact" "200" "Case exact lowercase"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/CASE/EXACT" "200" "Case exact uppercase"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/Case/Exact" "200" "Case exact mixed"; then ((PASS++)); else ((FAIL++)); fi

# Case insensitive prefix
if test_route "/case/prefix" "200" "Case prefix lowercase"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/CASE/PREFIX-test" "200" "Case prefix uppercase"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/Case/Prefix/sub" "200" "Case prefix mixed"; then ((PASS++)); else ((FAIL++)); fi

# Case insensitive directory
if test_route "/case/dir/users" "200" "Case dir lowercase"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/CASE/DIR/admin" "200" "Case dir uppercase"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/Case/Dir/settings" "200" "Case dir mixed"; then ((PASS++)); else ((FAIL++)); fi

# Case insensitive extension
if test_route "/case/files/doc.pdf" "200" "Case ext lowercase"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/CASE/FILES/DOC.PDF" "200" "Case ext uppercase"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/Case/Files/Doc.Pdf" "200" "Case ext mixed"; then ((PASS++)); else ((FAIL++)); fi

echo ""
echo "Testing special matchers:"

# Test POST to catch-all (all() matcher)
echo -n "Testing POST /any/path (all matcher) ... "
response=$(curl -s -X POST -w "HTTPSTATUS:%{http_code}" "$BASE_URL/any/path" 2>/dev/null)
status_code=$(echo "$response" | grep -o "HTTPSTATUS:[0-9]*" | cut -d: -f2)
if [ "$status_code" = "200" ]; then
    echo "✅ PASS ($status_code)"
    ((PASS++))
else
    echo "❌ FAIL (expected 200, got $status_code)"
    ((FAIL++))
fi

# Check if regex is enabled by testing the server
echo ""
echo "Checking for regex support..."
regex_test=$(curl -s "$BASE_URL/user/123" 2>/dev/null)
if curl -s -w "%{http_code}" "$BASE_URL/user/123" 2>/dev/null | grep -q "200"; then
    echo "Regex support detected - testing traditional regex routes:"
    if test_route "/user/123" "200" "Traditional regex user ID"; then ((PASS++)); else ((FAIL++)); fi
    if test_route "/user/456" "200" "Traditional regex user ID 2"; then ((PASS++)); else ((FAIL++)); fi
    if test_route "/blog/2023/10/15" "200" "Traditional regex blog date"; then ((PASS++)); else ((FAIL++)); fi
    if test_route "/blog/2024/12/25" "200" "Traditional regex blog date 2"; then ((PASS++)); else ((FAIL++)); fi

    echo "Testing AsyncURIMatcher regex factory methods:"
    if test_route "/factory/user/123" "200" "Factory regex user ID"; then ((PASS++)); else ((FAIL++)); fi
    if test_route "/factory/user/789" "200" "Factory regex user ID 2"; then ((PASS++)); else ((FAIL++)); fi
    if test_route "/factory/blog/2023/10/15" "200" "Factory regex blog date"; then ((PASS++)); else ((FAIL++)); fi
    if test_route "/factory/blog/2024/12/31" "200" "Factory regex blog date 2"; then ((PASS++)); else ((FAIL++)); fi

    # Case insensitive regex
    if test_route "/factory/search/hello" "200" "Factory regex search lowercase"; then ((PASS++)); else ((FAIL++)); fi
    if test_route "/FACTORY/SEARCH/WORLD" "200" "Factory regex search uppercase"; then ((PASS++)); else ((FAIL++)); fi
    if test_route "/Factory/Search/Test" "200" "Factory regex search mixed"; then ((PASS++)); else ((FAIL++)); fi

    # Test regex validation
    if test_route "/user/abc" "404" "Invalid regex (letters instead of numbers)"; then ((PASS++)); else ((FAIL++)); fi
    if test_route "/blog/23/10/15" "404" "Invalid regex (2-digit year)"; then ((PASS++)); else ((FAIL++)); fi
    if test_route "/factory/user/abc" "404" "Factory regex invalid (letters)"; then ((PASS++)); else ((FAIL++)); fi
else
    echo "Regex support not detected (compile with ASYNCWEBSERVER_REGEX to enable)"
fi

echo ""
echo "Testing routes that should fail (404 Not Found):"

if test_route "/nonexistent" "404" "Non-existent route"; then ((PASS++)); else ((FAIL++)); fi

# Test factory exact vs traditional behavior difference
if test_route "/factory/exact/sub" "404" "Factory exact should NOT match subpaths"; then ((PASS++)); else ((FAIL++)); fi

# Test factory directory requires trailing slash
if test_route "/factory/dir" "404" "Factory directory should NOT match without trailing slash"; then ((PASS++)); else ((FAIL++)); fi

# Test extension mismatch
if test_route "/factory/files/doc.pdf" "404" "Factory extension mismatch (.pdf vs .txt)"; then ((PASS++)); else ((FAIL++)); fi

# Test case sensitive when flag not used
if test_route "/exact" "200" "Traditional exact lowercase"; then ((PASS++)); else ((FAIL++)); fi
if test_route "/EXACT" "404" "Traditional exact should be case sensitive"; then ((PASS++)); else ((FAIL++)); fi

echo ""
echo "=================================="
echo "Test Results:"
echo "✅ Passed: $PASS"
echo "❌ Failed: $FAIL"
echo "Total: $((PASS + FAIL))"

if [ $FAIL -eq 0 ]; then
    echo ""
    echo "🎉 All tests passed! URI matching is working correctly."
    exit 0
else
    echo ""
    echo "❌ Some tests failed. Check the server and routes."
    exit 1
fi
