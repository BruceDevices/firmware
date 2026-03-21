function $(s) { return document.querySelector(s); }
const IS_DEV = (window.location.host === "127.0.0.1:8080");

const T = {
  master: $('#t'),
  fileRow: function () {
    const tmp = document.createElement('template');
    tmp.innerHTML = this.master.content.querySelector('table tr.file-row').outerHTML;
    return tmp.content;
  },
  pathRow: function () {
    const tmp = document.createElement('template');
    tmp.innerHTML = this.master.content.querySelector('table tr.path-row').outerHTML;
    return tmp.content;
  },
  uploadLoading: function () {
    const tmp = document.createElement('template');
    tmp.innerHTML = this.master.content.querySelector('.upload-loading').outerHTML;
    return tmp.content;
  }
};

const EXECUTABLE = {
  ir: "ir tx_from_file",
  sub: "subghz tx_from_file",
  js: "js run_from_file",
  bjs: "js run_from_file",
  txt: "badusb run_from_file",
  mp3: "play",
  wav: "play"
};

// Toast notification system
const Toast = {
  show: function (message, type, duration) {
    type = type || 'success';
    duration = duration || 3000;
    const container = $('#toast-container');
    const toast = document.createElement('div');
    toast.className = 'toast ' + type;
    toast.textContent = message;
    container.appendChild(toast);
    setTimeout(function () {
      toast.classList.add('removing');
      setTimeout(function () { toast.remove(); }, 200);
    }, duration);
  }
};

// Dialog system with animations
const Dialog = {
  _bg: function (show) {
    var bg = $(".dialog-background");
    var dialogs = document.querySelectorAll(".dialog");
    dialogs.forEach(function (d) { d.classList.add("hidden"); });

    if (show) {
      bg.classList.remove("hidden");
      bg.offsetHeight; // force reflow
      bg.classList.add("visible");
    } else {
      bg.classList.remove("visible");
      setTimeout(function () {
        if (!bg.classList.contains("visible")) {
          bg.classList.add("hidden");
        }
      }, 250);
    }
  },
  show: function (dialogName) {
    this._bg(true);
    $(".dialog." + dialogName).classList.remove("hidden");
  },
  hide: function () {
    this._bg(false);
    this.loading.hide();
    if (currentDrive && currentPath) updateURL(currentDrive, currentPath, null);
  },
  loading: {
    show: function (message) {
      $(".loading-area").classList.remove("hidden");
      $(".loading-area .text").textContent = message || "Loading...";
    },
    hide: function () {
      $(".loading-area").classList.add("hidden");
    }
  },
  showOneInput: function (name, inputVal, data) {
    var dbForm = {
      renameFolder: { title: "Rename Folder", label: "New folder name:", action: "Rename" },
      renameFile: { title: "Rename File", label: "New file name:", action: "Rename" },
      createFolder: { title: "New Folder", label: "Folder name:", action: "Create" },
      createFile: { title: "New File", label: "File name:", action: "Create" },
      serial: { title: "Serial Command", label: "Command:", action: "Run" }
    };

    var config = dbForm[name];
    if (!config) return;

    var dialog = $(".dialog.oinput");
    dialog.setAttribute("data-cache", data);
    dialog.querySelector(".oinput-title").textContent = config.title;
    dialog.querySelector(".oinput-label").textContent = config.label;
    dialog.querySelector("#oinput-input").value = inputVal;
    dialog.querySelector(".act-save-oinput-file").textContent = config.action;
    this.show('oinput');
    dialog.querySelector("#oinput-input").select();
    return dialog;
  }
};

function handleAuthError() {
  if (confirm("Session expired. Go to login page?")) {
    window.location.href = "/";
  } else {
    Dialog.loading.hide();
  }
}

async function requestGet(url, data) {
  return new Promise(function (resolve, reject) {
    var req = new XMLHttpRequest();
    var realUrl = url;
    if (IS_DEV) realUrl = "/bruce" + url;
    if (data) {
      var urlParams = new URLSearchParams(data);
      realUrl += "?" + urlParams.toString();
    }
    req.open("GET", realUrl, true);
    req.onload = function () {
      if (req.status >= 200 && req.status < 300) resolve(req.responseText);
      else if (req.status === 401) { handleAuthError(); reject(new Error("Unauthorized (401)")); }
      else reject(new Error("Request failed: " + req.status));
    };
    req.onerror = function () { reject(new Error("Network error")); };
    req.send();
  });
}

async function requestPost(url, data) {
  return new Promise(function (resolve, reject) {
    var fd = new FormData();
    for (var key in data) {
      if (data.hasOwnProperty(key)) fd.append(key, data[key]);
    }
    var realUrl = url;
    if (IS_DEV) realUrl = "/bruce" + url;
    var req = new XMLHttpRequest();
    req.open("POST", realUrl, true);
    req.onload = function () {
      if (req.status >= 200 && req.status < 300) resolve(req.responseText);
      else if (req.status === 401) { handleAuthError(); reject(new Error("Unauthorized (401)")); }
      else reject(new Error("Request failed: " + req.status));
    };
    req.onerror = function () { reject(new Error("Network error")); };
    req.send(fd);
  });
}

function stringToId(str) {
  var hash = 0, i, chr;
  if (str.length === 0) return hash.toString();
  for (i = 0; i < str.length; i++) {
    chr = str.charCodeAt(i);
    hash = ((hash << 5) - hash) + chr;
    hash |= 0;
  }
  return 'id_' + Math.abs(hash);
}

function calcHash(str) {
  var hash = 5381;
  str = str.replace(/\r\n/g, '\n').replace(/\r/g, '\n');
  for (var i = 0; i < str.length; i++) {
    hash = ((hash << 5) + hash) ^ str.charCodeAt(i);
    hash = hash >>> 0;
  }
  return hash.toString(16).padStart(8, '0');
}

function getSerialCommand(fileName) {
  var ext = fileName.split('.');
  if (ext.length > 1) {
    ext = ext[ext.length - 1].toLowerCase();
    return EXECUTABLE[ext];
  }
  return undefined;
}

function parseStorageBytes(str) {
  var parts = str.trim().split(' ');
  if (parts.length < 2) return 0;
  var num = parseFloat(parts[0]);
  var unit = parts[1].toUpperCase();
  var mult = { 'B': 1, 'KB': 1024, 'MB': 1048576, 'GB': 1073741824 };
  return num * (mult[unit] || 1);
}

// Upload system
var _queueUpload = [];
var _runningUpload = false;
var _uploadItems = []; // {file, id, status:'pending'|'uploading'|'done'|'error', pct:0}
var _uploadPage = 0;
var _uploadPerPage = 8;
var _uploadStartTime = 0;
var _uploadedBytes = 0;
var _totalBytes = 0;
var _uploadedCount = 0;

function resetUploadState() {
  _uploadItems = [];
  _uploadPage = 0;
  _uploadedBytes = 0;
  _totalBytes = 0;
  _uploadedCount = 0;
  _uploadStartTime = 0;
}

function addUploadItem(file) {
  var filename = file.webkitRelativePath || file.name;
  var id = stringToId(filename + '_' + _uploadItems.length);
  _uploadItems.push({ file: file, id: id, name: filename, status: 'pending', pct: 0 });
  _totalBytes += file.size;
}

function renderUploadPage() {
  var body = $('#upload-list');
  body.innerHTML = '';
  var totalPages = Math.max(1, Math.ceil(_uploadItems.length / _uploadPerPage));
  if (_uploadPage >= totalPages) _uploadPage = totalPages - 1;
  if (_uploadPage < 0) _uploadPage = 0;

  var start = _uploadPage * _uploadPerPage;
  var end = Math.min(start + _uploadPerPage, _uploadItems.length);
  for (var i = start; i < end; i++) {
    var item = _uploadItems[i];
    var el = T.uploadLoading();
    var row = el.querySelector('.upload-loading');
    row.setAttribute('data-uid', item.id);
    if (item.status !== 'pending') row.classList.add(item.status === 'uploading' ? 'active' : item.status);
    el.querySelector('.upload-name').textContent = item.name;
    el.querySelector('.upload-name').setAttribute('title', item.name);
    var bar = el.querySelector('.bar');
    bar.setAttribute('id', item.id);
    bar.style.width = item.pct + '%';
    var pctLabel = el.querySelector('.upload-pct');
    if (item.status === 'done') pctLabel.textContent = '100%';
    else if (item.status === 'error') pctLabel.textContent = 'ERR';
    else if (item.status === 'uploading') pctLabel.textContent = Math.round(item.pct) + '%';
    else pctLabel.textContent = '';
    body.appendChild(el);
  }

  // Auto-scroll to page with active upload
  var activeIdx = _uploadItems.findIndex(function(it) { return it.status === 'uploading'; });
  if (activeIdx >= 0) {
    var activePage = Math.floor(activeIdx / _uploadPerPage);
    if (activePage !== _uploadPage) {
      _uploadPage = activePage;
      renderUploadPage();
      return;
    }
  }

  updateUploadPagination();
}

function updateUploadPagination() {
  var totalPages = Math.max(1, Math.ceil(_uploadItems.length / _uploadPerPage));
  $('#upload-page-info').textContent = (_uploadPage + 1) + ' / ' + totalPages;
  $('#upload-prev').disabled = _uploadPage <= 0;
  $('#upload-next').disabled = _uploadPage >= totalPages - 1;
  // Hide pagination if only one page
  var pagesEl = $('#upload-pages');
  if (totalPages <= 1) pagesEl.style.display = 'none';
  else pagesEl.style.display = '';
}

function updateUploadStats(currentFilePct, currentFileSize) {
  var done = _uploadedCount;
  var total = _uploadItems.length;
  var partialBytes = (currentFilePct / 100) * currentFileSize;
  var transferred = _uploadedBytes + partialBytes;

  // Counter
  $('#upload-counter').textContent = done + ' / ' + total;

  // Overall progress
  var overallPct = _totalBytes > 0 ? (transferred / _totalBytes) * 100 : 0;
  $('#upload-overall-fill').style.width = Math.min(100, overallPct).toFixed(1) + '%';

  // Speed & ETA
  var elapsed = (Date.now() - _uploadStartTime) / 1000;
  if (elapsed > 0.5 && transferred > 0) {
    var speed = transferred / elapsed;
    var remaining = _totalBytes - transferred;
    var etaSec = remaining / speed;

    // Format speed
    var speedStr;
    if (speed >= 1048576) speedStr = (speed / 1048576).toFixed(1) + ' MB/s';
    else if (speed >= 1024) speedStr = (speed / 1024).toFixed(0) + ' KB/s';
    else speedStr = Math.round(speed) + ' B/s';
    $('#upload-speed').textContent = speedStr;

    // Format ETA
    var etaStr;
    if (etaSec < 1) etaStr = 'ETA: < 1s';
    else if (etaSec < 60) etaStr = 'ETA: ' + Math.ceil(etaSec) + 's';
    else if (etaSec < 3600) etaStr = 'ETA: ' + Math.floor(etaSec / 60) + 'm ' + Math.ceil(etaSec % 60) + 's';
    else etaStr = 'ETA: ' + Math.floor(etaSec / 3600) + 'h ' + Math.floor((etaSec % 3600) / 60) + 'm';
    $('#upload-eta').textContent = etaStr;
  } else {
    $('#upload-speed').textContent = '--';
    $('#upload-eta').textContent = 'ETA: calculating...';
  }
}

function appendFileToQueue(files) {
  if (_uploadItems.length === 0) resetUploadState();
  Dialog.show('upload');
  for (var i = 0; i < files.length; i++) addUploadItem(files[i]);
  updateUploadStats(0, 0);
  renderUploadPage();
}

async function appendDroppedFiles(entry) {
  return new Promise(function (resolve) {
    if (entry.isFile) {
      entry.file(function (file) {
        var fileWithPath = new File([file], entry.fullPath.substring(1), { type: file.type });
        addUploadItem(fileWithPath);
        _queueUpload.push(fileWithPath);
        resolve();
      });
    } else if (entry.isDirectory) {
      var reader = entry.createReader();
      var allEntries = [];
      var readBatch = function () {
        reader.readEntries(function (entries) {
          if (entries.length === 0) {
            var proms = [];
            for (var i = 0; i < allEntries.length; i++) proms.push(appendDroppedFiles(allEntries[i]));
            Promise.all(proms).then(resolve);
          } else {
            allEntries = allEntries.concat(Array.from(entries));
            readBatch();
          }
        });
      };
      readBatch();
    }
  });
}

function updateItemUI(item) {
  var row = document.querySelector('[data-uid="' + item.id + '"]');
  if (!row) return;
  row.className = 'upload-loading' + (item.status === 'uploading' ? ' active' : item.status !== 'pending' ? ' ' + item.status : '');
  var bar = row.querySelector('.bar');
  if (bar) bar.style.width = item.pct + '%';
  var pct = row.querySelector('.upload-pct');
  if (pct) {
    if (item.status === 'done') pct.textContent = '100%';
    else if (item.status === 'error') pct.textContent = 'ERR';
    else if (item.status === 'uploading') pct.textContent = Math.round(item.pct) + '%';
    else pct.textContent = '';
  }
}

async function uploadFile() {
  if (_queueUpload.length === 0) {
    _runningUpload = false;
    // Final stats
    updateUploadStats(0, 0);
    $('#upload-counter').textContent = _uploadedCount + ' / ' + _uploadItems.length;
    $('#upload-overall-fill').style.width = '100%';
    $('#upload-eta').textContent = 'Complete!';
    setTimeout(function() {
      resetUploadState();
      $('#upload-list').innerHTML = '';
      fetchSystemInfo();
      fetchFiles(currentDrive, currentPath);
      Dialog.hide();
      Toast.show("Upload complete", "success");
    }, 800);
    return;
  }

  return new Promise(function (resolve, reject) {
    _runningUpload = true;
    if (_uploadStartTime === 0) _uploadStartTime = Date.now();

    var file = _queueUpload.shift();
    var filename = file.webkitRelativePath || file.name;

    // Find matching upload item
    var item = null;
    for (var i = 0; i < _uploadItems.length; i++) {
      if (_uploadItems[i].status === 'pending' && _uploadItems[i].name === filename) {
        item = _uploadItems[i]; break;
      }
    }
    if (!item) {
      // Fallback: find first pending
      for (var j = 0; j < _uploadItems.length; j++) {
        if (_uploadItems[j].status === 'pending') { item = _uploadItems[j]; break; }
      }
    }
    if (item) {
      item.status = 'uploading';
      item.pct = 0;
      renderUploadPage();
    }

    var fd = new FormData();
    fd.append("file", file, filename);
    fd.append("folder", currentPath);
    fd.append("fs", currentDrive);

    var realUrl = "/upload";
    if (IS_DEV) realUrl = "/bruce" + realUrl;
    var req = new XMLHttpRequest();
    req.upload.onprogress = function (e) {
      if (e.lengthComputable) {
        var pct = (e.loaded / e.total) * 100;
        if (item) {
          item.pct = pct;
          updateItemUI(item);
        }
        updateUploadStats(pct, file.size);
      }
    };
    req.onload = function () {
      if (item) {
        item.status = (req.status >= 200 && req.status < 300) ? 'done' : 'error';
        item.pct = 100;
        updateItemUI(item);
      }
      _uploadedCount++;
      _uploadedBytes += file.size;
      updateUploadStats(0, 0);
      renderUploadPage();
      uploadFile();
      if (req.status >= 200 && req.status < 300) resolve(req.responseText);
      else reject();
    };
    req.onabort = function () {
      if (item) { item.status = 'error'; updateItemUI(item); }
      reject();
    };
    req.onerror = function () {
      if (item) { item.status = 'error'; updateItemUI(item); }
      reject();
    };
    req.open("POST", realUrl, true);
    req.send(fd);
  });
}

async function runCommand(cmd) {
  Dialog.loading.show('Running command...');
  try {
    await requestPost("/cm", { cmnd: cmd });
    Toast.show("Command executed", "success");
  } catch (error) {
    Toast.show("Command failed: " + error.message, "error");
  } finally {
    Dialog.loading.hide();
  }
}

// Editor
function updateLineNumbers() {
  var textarea = $(".dialog.editor .file-content");
  var lineNumbers = $(".dialog.editor .line-numbers");
  if (!textarea || !lineNumbers) return;
  var lines = textarea.value.split('\n');
  var nums = '';
  for (var i = 1; i <= lines.length; i++) nums += i + '\n';
  lineNumbers.textContent = nums;
}

function syncScrolling() {
  var textarea = $(".dialog.editor .file-content");
  var lineNumbers = $(".dialog.editor .line-numbers");
  if (textarea && lineNumbers) lineNumbers.scrollTop = textarea.scrollTop;
}

function isModified(target) {
  return target.getAttribute("data-hash") !== calcHash(target.value);
}

async function saveEditorFile(runFile) {
  Dialog.loading.show('Saving...');
  var editor = $(".dialog.editor .file-content");
  var filename = $(".dialog.editor .editor-file-name").textContent.trim();
  if (isModified(editor)) {
    $(".act-save-edit-file").disabled = true;
    editor.setAttribute("data-hash", calcHash(editor.value));
    await requestPost("/edit", { fs: currentDrive, name: filename, content: editor.value });
    Toast.show("File saved", "success");
  }
  if (runFile) {
    var serial = getSerialCommand(filename);
    if (serial !== undefined) await runCommand(serial + ' "' + filename + '"');
  }
  Dialog.loading.hide();
}

// Multi-select system
var selectedFiles = new Set();

function updateMultiBar() {
  var bar = $('#multi-bar');
  var count = selectedFiles.size;
  if (count > 0) {
    bar.classList.remove('hidden');
    $('#selected-count').textContent = count + ' selected';
  } else {
    bar.classList.add('hidden');
  }
  var checkAll = $('#check-all');
  var allChecks = document.querySelectorAll('.file-check');
  if (allChecks.length > 0 && count === allChecks.length) {
    checkAll.checked = true;
  } else {
    checkAll.checked = false;
  }
}

function clearSelection() {
  selectedFiles.clear();
  document.querySelectorAll('.file-check').forEach(function (cb) { cb.checked = false; });
  document.querySelectorAll('tr.selected').forEach(function (r) { r.classList.remove('selected'); });
  updateMultiBar();
}

async function deleteSelected() {
  var files = Array.from(selectedFiles);
  if (files.length === 0) return;
  if (!confirm("Delete " + files.length + " item(s)?\n\nTHIS CANNOT BE UNDONE!")) return;

  Dialog.loading.show('Deleting ' + files.length + ' items...');
  var errors = 0;
  for (var i = 0; i < files.length; i++) {
    Dialog.loading.show('Deleting (' + (i + 1) + '/' + files.length + ')...');
    try {
      await requestGet("/file", { fs: currentDrive, action: 'delete', name: files[i] });
    } catch (e) { errors++; }
  }
  Dialog.loading.hide();
  clearSelection();
  fetchSystemInfo();
  fetchFiles(currentDrive, currentPath);
  if (errors > 0) Toast.show(errors + " deletion(s) failed", "error");
  else Toast.show(files.length + " item(s) deleted", "success");
}

// File list rendering and state
var _rawFileList = '';
var _parsedFiles = [];
var _sortCol = 'name';
var _sortDir = 1; // 1 = asc, -1 = desc

function parseFileList(fileList) {
  var parsed = [];
  fileList.split("\n").forEach(function (line) {
    var parts = line.split(":");
    var type = parts[0];
    var name = parts[1];
    var size = parts.slice(2).join(":");
    if (size === undefined) return;
    parsed.push({ type: type, name: name, size: size, rawSize: parseSizeToBytes(size) });
  });
  return parsed;
}

function parseSizeToBytes(sizeStr) {
  if (!sizeStr || sizeStr === '-') return -1;
  var num = parseFloat(sizeStr);
  if (isNaN(num)) return 0;
  var s = sizeStr.toUpperCase();
  if (s.indexOf('GB') >= 0) return num * 1073741824;
  if (s.indexOf('MB') >= 0) return num * 1048576;
  if (s.indexOf('KB') >= 0) return num * 1024;
  return num;
}

function sortFiles(files) {
  return files.slice().sort(function (a, b) {
    // Parent dir always first
    if (a.type === 'pa') return -1;
    if (b.type === 'pa') return 1;
    // Folders before files
    if (a.type !== b.type) return b.type.localeCompare(a.type);

    if (_sortCol === 'size') {
      return (a.rawSize - b.rawSize) * _sortDir;
    }
    return a.name.toLowerCase().localeCompare(b.name.toLowerCase()) * _sortDir;
  });
}

function renderFileRow(fileList) {
  _rawFileList = fileList;
  _parsedFiles = parseFileList(fileList);
  renderParsedFiles();
}

function renderParsedFiles() {
  var tbody = $("table.explorer tbody");
  tbody.innerHTML = "";
  selectedFiles.clear();
  updateMultiBar();

  var search = ($('#search-files') || {}).value || '';
  search = search.toLowerCase().trim();

  var sorted = sortFiles(_parsedFiles);
  var visibleCount = 0;

  sorted.forEach(function (item) {
    var e;
    var dPath = ((currentPath.endsWith("/") ? currentPath : currentPath + "/") + item.name).replace(/\/\//g, "/");

    if (item.type === "pa") {
      if (dPath === "/") return;
      e = T.pathRow();
      var preFolder = currentPath.substring(0, currentPath.lastIndexOf('/'));
      if (preFolder === "") preFolder = "/";
      e.querySelector(".path-row").setAttribute("data-path", preFolder);
      visibleCount++;
    } else if (item.type === "Fi") {
      if (search && item.name.toLowerCase().indexOf(search) === -1) return;
      e = T.fileRow();
      e.querySelector('.file-row').setAttribute("data-file", dPath);
      e.querySelector('.act-rename').setAttribute("data-action", "renameFile");
      e.querySelector(".col-name").classList.add("act-edit-file");
      e.querySelector(".col-name").textContent = item.name;
      e.querySelector(".col-name").setAttribute("title", item.name);
      e.querySelector(".col-size").textContent = item.size;
      e.querySelector(".col-action").classList.add("type-file");

      var downloadUrl = '/file?fs=' + currentDrive + '&name=' + encodeURIComponent(dPath) + '&action=download';
      if (IS_DEV) downloadUrl = "/bruce" + downloadUrl;
      e.querySelector(".act-download").setAttribute("download", item.name);
      e.querySelector(".act-download").setAttribute("href", downloadUrl);

      var serialCmd = getSerialCommand(item.name);
      if (serialCmd) {
        e.querySelector(".act-play").setAttribute("data-cmd", serialCmd + ' "' + dPath + '"');
        e.querySelector(".col-action").classList.add("executable");
      }
      visibleCount++;
    } else if (item.type === "Fo") {
      if (search && item.name.toLowerCase().indexOf(search) === -1) return;
      e = T.fileRow();
      e.querySelector(".col-name").classList.add("act-browse");
      e.querySelector('.file-row').setAttribute("data-path", dPath);
      e.querySelector('.act-rename').setAttribute("data-action", "renameFolder");
      e.querySelector(".col-name").textContent = item.name;
      e.querySelector(".col-name").setAttribute("title", item.name);
      e.querySelector(".col-action").classList.add("type-folder");
      visibleCount++;
    }

    if (e) tbody.appendChild(e);
  });

  var empty = $('#empty-state');
  if (visibleCount === 0 && search) {
    empty.classList.remove('hidden');
  } else {
    empty.classList.add('hidden');
  }

  updateSortIcons();
}

function updateSortIcons() {
  document.querySelectorAll('.sort-icon').forEach(function (el) { el.textContent = ''; });
  var th = $('th[data-sort="' + _sortCol + '"]');
  if (th) {
    th.querySelector('.sort-icon').textContent = _sortDir === 1 ? ' \u25B2' : ' \u25BC';
  }
}

// State
var sdCardAvailable = false;
var currentDrive;
var currentPath;
var btnRefreshFolder = $("#refresh-folder");

// URL management
function updateURL(drive, path, editFile) {
  var params = new URLSearchParams();
  if (drive) params.set('drive', drive);
  if (path && path !== '/') params.set('path', path);
  if (editFile) params.set('edit', editFile);
  var newURL = window.location.pathname + (params.toString() ? '?' + params.toString() : '');
  window.history.replaceState({ drive: drive, path: path, editFile: editFile }, '', newURL);
}

function getURLParams() {
  var params = new URLSearchParams(window.location.search);
  return {
    drive: params.get('drive'),
    path: params.get('path') || '/',
    editFile: params.get('edit')
  };
}

async function fetchFiles(drive, path) {
  btnRefreshFolder.classList.add("reloading");
  $("table.explorer tbody").innerHTML = '<tr><td colspan="4" style="text-align:center;padding:20px;color:var(--text-muted)">Loading...</td></tr>';
  currentDrive = drive;
  currentPath = path;

  var urlParams = getURLParams();
  updateURL(drive, path, urlParams.editFile);

  var prev = document.querySelector('.act-browse.active');
  if (prev) prev.classList.remove("active");
  var card = document.querySelector('.act-browse[data-drive="' + drive + '"]');
  if (card) card.classList.add("active");
  $(".current-path").textContent = drive + ":/" + path;
  clearSelection();

  var req = await requestGet("/listfiles", { fs: drive, folder: path });
  renderFileRow(req);
  btnRefreshFolder.classList.remove("reloading");
}

async function fetchSystemInfo() {
  Dialog.loading.show('Fetching system info...');
  try {
    var req = await requestGet("/systeminfo");
    var info = JSON.parse(req);
    $(".bruce-version").textContent = info.BRUCE_VERSION;
    $(".free-sd span").textContent = info.SD.used + ' / ' + info.SD.total;
    $(".free-fs span").textContent = info.LittleFS.used + ' / ' + info.LittleFS.total;
    sdCardAvailable = info.SD.total !== '0 B';

    // Storage fill bars
    var sdUsed = parseStorageBytes(info.SD.used);
    var sdTotal = parseStorageBytes(info.SD.total);
    var fsUsed = parseStorageBytes(info.LittleFS.used);
    var fsTotal = parseStorageBytes(info.LittleFS.total);
    var sdFill = document.getElementById('sd-fill');
    var fsFill = document.getElementById('fs-fill');
    if (sdFill) sdFill.style.width = (sdTotal > 0 ? Math.min(100, (sdUsed / sdTotal) * 100) : 0) + '%';
    if (fsFill) fsFill.style.width = (fsTotal > 0 ? Math.min(100, (fsUsed / fsTotal) * 100) : 0) + '%';
  } catch (e) {
    Toast.show("Failed to fetch system info", "error");
  }
  Dialog.loading.hide();
}

// Navigator
async function openNavigator() {
  Dialog.show('navigator');
  await reloadScreen();
  autoReloadScreen();
}

var SCREEN_NAVIGATING = false;
async function runNavigation(direction) {
  if (SCREEN_NAVIGATING) return;
  SCREEN_NAVIGATING = true;
  try {
    drawCanvasLoading();
    await requestPost("/cm", { cmnd: "nav " + direction.toLowerCase() });
    await reloadScreen();
  } catch (error) {
    Toast.show("Navigation failed: " + error.message, "error");
  } finally {
    SCREEN_NAVIGATING = false;
  }
}

var btnForceReload = $("#force-reload");
var SCREEN_RELOAD = false;
async function reloadScreen() {
  if (SCREEN_RELOAD) return;
  SCREEN_RELOAD = true;
  btnForceReload.classList.add("reloading");
  try {
    var binResponse = await fetch((IS_DEV ? "/bruce" : "") + "/getscreen");
    var arrayBuffer = await binResponse.arrayBuffer();
    var screenData = new Uint8Array(arrayBuffer);
    await renderTFT(screenData);
  } catch (error) {
    Toast.show("Screen reload failed", "error");
  } finally {
    btnForceReload.classList.remove("reloading");
    SCREEN_RELOAD = false;
  }
}

var eConfigAutoReload = $("#navigator-auto-reload");
var AUTO_RELOAD_SCREEN = null;
async function taskReloader() {
  var timer = parseInt(eConfigAutoReload.value);
  var navigatorOpen = $(".dialog.navigator:not(.hidden)");
  if (timer <= 0 || !navigatorOpen) {
    if (AUTO_RELOAD_SCREEN) { clearTimeout(AUTO_RELOAD_SCREEN); AUTO_RELOAD_SCREEN = null; }
    return;
  }
  await reloadScreen();
  setTimeout(taskReloader, timer);
}
async function autoReloadScreen() {
  var timer = parseInt(eConfigAutoReload.value);
  if (AUTO_RELOAD_SCREEN) { clearTimeout(AUTO_RELOAD_SCREEN); AUTO_RELOAD_SCREEN = null; }
  if (timer > 0) taskReloader();
}

// TFT Rendering — preserved exactly
var loadingDrawn = false;
var imageCache = {};
async function renderTFT(data) {
  loadingDrawn = false;
  var canvas = $("#navigator-screen");
  var ctx = canvas.getContext("2d");

  var loadImage = async function (url) {
    if (imageCache[url]) return imageCache[url];
    return new Promise(function (resolve, reject) {
      var img = new Image();
      img.onload = function () { imageCache[url] = img; resolve(img); };
      img.onerror = function (err) { reject(err); };
      img.src = url;
    });
  };

  var drawImageCached = async function (img_url, input) {
    if (IS_DEV) img_url = "/bruce" + img_url;
    var img = await loadImage(img_url);
    var drawX = input.x;
    var drawY = input.y;
    if (input.center === 1) {
      drawX += (canvas.width - img.width) / 2;
      drawY += (canvas.height - img.height) / 2;
    }
    ctx.drawImage(img, drawX, drawY);
  };

  var color565toCSS = function (color565) {
    var r = ((color565 >> 11) & 0x1F) * 255 / 31;
    var g = ((color565 >> 5) & 0x3F) * 255 / 63;
    var b = (color565 & 0x1F) * 255 / 31;
    return "rgb(" + r + "," + g + "," + b + ")";
  };

  var drawRoundRect = function (ctx, input, fill) {
    var x = input.x, y = input.y, w = input.w, h = input.h, r = input.r;
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.arcTo(x + w, y, x + w, y + h, r);
    ctx.arcTo(x + w, y + h, x, y + h, r);
    ctx.arcTo(x, y + h, x, y, r);
    ctx.arcTo(x, y, x + w, y, r);
    ctx.closePath();
    if (fill) ctx.fill(); else ctx.stroke();
  };

  var startData = 0;
  var getByteValue = function (dataType) {
    if (dataType === 'int8') {
      return data[startData++];
    } else if (dataType === 'int16') {
      var value = (data[startData] << 8) | data[startData + 1];
      startData += 2;
      return value;
    } else if (dataType.startsWith("s")) {
      var strLength = parseInt(dataType.substring(1));
      var strBytes = data.slice(startData, startData + strLength);
      startData += strLength;
      return new TextDecoder().decode(strBytes);
    }
  };

  var byteToObject = function (fn, size) {
    var keysMap = {
      0: ["fg"],
      1: ["x", "y", "w", "h", "fg"],
      2: ["x", "y", "w", "h", "fg"],
      3: ["x", "y", "w", "h", "r", "fg"],
      4: ["x", "y", "w", "h", "r", "fg"],
      5: ["x", "y", "r", "fg"],
      6: ["x", "y", "r", "fg"],
      7: ["x", "y", "x2", "y2", "x3", "y3", "fg"],
      8: ["x", "y", "x2", "y2", "x3", "y3", "fg"],
      9: ["x", "y", "rx", "ry", "fg"],
      10: ["x", "y", "rx", "ry", "fg"],
      11: ["x", "y", "x1", "y1", "fg"],
      12: ["x", "y", "r", "ir", "startAngle", "endAngle", "fg", "bg"],
      13: ["x", "y", "bx", "by", "wd", "fg", "bg"],
      14: ["x", "y", "size", "fg", "bg", "txt"],
      15: ["x", "y", "size", "fg", "bg", "txt"],
      16: ["x", "y", "size", "fg", "bg", "txt"],
      17: ["x", "y", "size", "fg", "bg", "txt"],
      18: ["x", "y", "center", "ms", "fs", "file"],
      20: ["x", "y", "h", "fg"],
      21: ["x", "y", "w", "fg"],
      99: ["w", "h", "rotation"]
    };

    var r = {};
    var lengthLeft = size - 3;
    for (var key of keysMap[fn]) {
      if (['txt', 'file'].includes(key)) {
        r[key] = getByteValue("s" + lengthLeft);
      } else if (['rotation', 'fs'].includes(key)) {
        lengthLeft -= 1;
        r[key] = getByteValue('int8');
        if (key === 'fs') r[key] = (r[key] === 0) ? "SD" : "FS";
      } else {
        lengthLeft -= 2;
        r[key] = getByteValue('int16');
      }
    }
    return r;
  };

  var offset = 0;
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  while (offset < data.length) {
    ctx.beginPath();
    if (data[offset] !== 0xAA) break;

    startData = offset + 1;
    var size = getByteValue('int8');
    var fn = getByteValue('int8');
    offset += size;

    var input = byteToObject(fn, size);
    ctx.lineWidth = 1;
    ctx.fillStyle = "black";
    ctx.strokeStyle = "black";
    switch (fn) {
      case 99:
        canvas.width = input.w;
        canvas.height = input.h;
      case 0:
        ctx.fillStyle = color565toCSS(input.fg);
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        break;
      case 1:
        ctx.strokeStyle = color565toCSS(input.fg);
        ctx.strokeRect(input.x, input.y, input.w, input.h);
        break;
      case 2:
        ctx.fillStyle = color565toCSS(input.fg);
        ctx.fillRect(input.x, input.y, input.w, input.h);
        break;
      case 3:
        ctx.strokeStyle = color565toCSS(input.fg);
        drawRoundRect(ctx, input, false);
        break;
      case 4:
        ctx.fillStyle = color565toCSS(input.fg);
        drawRoundRect(ctx, input, true);
        break;
      case 5:
        ctx.strokeStyle = color565toCSS(input.fg);
        ctx.arc(input.x, input.y, input.r, 0, Math.PI * 2);
        ctx.stroke();
        break;
      case 6:
        ctx.fillStyle = color565toCSS(input.fg);
        ctx.arc(input.x, input.y, input.r, 0, Math.PI * 2);
        ctx.fill();
        break;
      case 7:
        ctx.strokeStyle = color565toCSS(input.fg);
        ctx.beginPath();
        ctx.moveTo(input.x, input.y);
        ctx.lineTo(input.x2, input.y2);
        ctx.lineTo(input.x3, input.y3);
        ctx.closePath();
        ctx.stroke();
        break;
      case 8:
        ctx.fillStyle = color565toCSS(input.fg);
        ctx.beginPath();
        ctx.moveTo(input.x, input.y);
        ctx.lineTo(input.x2, input.y2);
        ctx.lineTo(input.x3, input.y3);
        ctx.closePath();
        ctx.fill();
        break;
      case 9:
        ctx.strokeStyle = color565toCSS(input.fg);
        ctx.beginPath();
        ctx.ellipse(input.x, input.y, input.rx, input.ry, 0, 0, Math.PI * 2);
        ctx.stroke();
        break;
      case 10:
        ctx.fillStyle = color565toCSS(input.fg);
        ctx.beginPath();
        ctx.ellipse(input.x, input.y, input.rx, input.ry, 0, 0, Math.PI * 2);
        ctx.fill();
        break;
      case 11:
        ctx.strokeStyle = color565toCSS(input.fg);
        ctx.moveTo(input.x, input.y);
        ctx.lineTo(input.x1, input.y1);
        ctx.stroke();
        break;
      case 12:
        ctx.strokeStyle = color565toCSS(input.fg);
        ctx.lineWidth = (input.r - input.ir) || 1;
        var sa = (input.startAngle + 90 || 0) * Math.PI / 180;
        var ea = (input.endAngle + 90 || 0) * Math.PI / 180;
        var radius = (input.r + input.ir) / 2;
        ctx.beginPath();
        ctx.arc(input.x, input.y, radius, sa, ea);
        ctx.stroke();
        break;
      case 13:
        ctx.strokeStyle = color565toCSS(input.fg);
        ctx.lineWidth = input.wd || 1;
        ctx.moveTo(input.x, input.y);
        ctx.lineTo(input.bx, input.by);
        ctx.stroke();
        break;
      case 14: case 15: case 16: case 17:
        if (input.bg == input.fg) input.bg = 0;
        ctx.fillStyle = color565toCSS(input.bg);
        input.txt = input.txt.replaceAll("\\n", "");
        var fw = input.size === 3 ? 13.5 : input.size === 2 ? 9 : 4.5;
        var o = 0;
        if (fn === 15) o = input.txt.length * fw;
        if (fn === 14) o = input.txt.length * fw / 2;
        ctx.fillRect(input.x - o, input.y, input.txt.length * fw, input.size * 8);
        ctx.fillStyle = color565toCSS(input.fg);
        ctx.font = (input.size * 8) + "px monospace";
        ctx.textBaseline = "top";
        ctx.textAlign = fn === 14 ? "center" : fn === 15 ? "right" : "left";
        ctx.fillText(input.txt, input.x, input.y);
        break;
      case 18:
        var url = "/file?fs=" + input.fs + "&name=" + encodeURIComponent(input.file) + "&action=image";
        await drawImageCached(url, input);
        break;
      case 19:
        ctx.fillStyle = color565toCSS(input.fg);
        ctx.fillRect(input.x, input.y, 1, 1);
        break;
      case 20:
        ctx.fillStyle = color565toCSS(input.fg);
        ctx.fillRect(input.x, input.y, 1, input.h);
        break;
      case 21:
        ctx.fillStyle = color565toCSS(input.fg);
        ctx.fillRect(input.x, input.y, input.w, 1);
        break;
    }
  }
}

function drawCanvasLoading() {
  if (loadingDrawn || !showNavigating) return;
  loadingDrawn = true;
  var canvas = $("#navigator-screen");
  var ctx = canvas.getContext("2d");
  ctx.save();
  ctx.globalAlpha = 0.8;
  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.globalAlpha = 1.0;
  ctx.fillStyle = "#fff";
  ctx.font = "bold 14px -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif";
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText("Navigating...", canvas.width / 2, canvas.height / 2);
  ctx.restore();
}

// Event setup
var oldTimerSession = sessionStorage.getItem("autoReload") || "0";
var timerOption = eConfigAutoReload.querySelector('option[value="' + oldTimerSession + '"]');
if (timerOption) timerOption.selected = true;
eConfigAutoReload.addEventListener("change", function () {
  autoReloadScreen();
  sessionStorage.setItem("autoReload", eConfigAutoReload.value);
});

btnForceReload.addEventListener("click", function (e) {
  e.preventDefault();
  drawCanvasLoading();
  reloadScreen();
});

// Mobile menu toggle
document.getElementById('mobile-menu-toggle').addEventListener('click', function () {
  document.getElementById('header-nav').classList.toggle('open');
});
// Close mobile menu on any nav button click
document.querySelectorAll('#header-nav .btn').forEach(function (btn) {
  btn.addEventListener('click', function () {
    document.getElementById('header-nav').classList.remove('open');
  });
});

// Drag and drop
window.ondragenter = function () { $(".upload-area").classList.remove("hidden"); };
$(".upload-area").ondragleave = function () { $(".upload-area").classList.add("hidden"); };
$(".upload-area").ondragover = function (e) { e.preventDefault(); };
$(".upload-area").ondrop = async function (e) {
  e.preventDefault();
  $(".upload-area").classList.add("hidden");
  var items = e.dataTransfer.items;
  if (!items || items.length === 0) return;
  // Collect all entries synchronously — the DataTransferItemList is
  // a live object that the browser invalidates after the first await.
  var entries = [];
  for (var k = 0; k < items.length; k++) {
    var entry = items[k].webkitGetAsEntry();
    if (entry) entries.push(entry);
  }
  if (entries.length === 0) return;
  if (_uploadItems.length === 0) resetUploadState();
  Dialog.show('upload');
  for (var j = 0; j < entries.length; j++) {
    await appendDroppedFiles(entries[j]);
  }
  renderUploadPage();
  updateUploadStats(0, 0);
  if (!_runningUpload) setTimeout(function () {
    if (_queueUpload.length === 0) return;
    uploadFile();
  }, 100);
};

document.querySelectorAll(".inp-uploader").forEach(function (el) {
  el.addEventListener("change", function (e) {
    var files = e.target.files;
    if (!files || files.length === 0) return;
    appendFileToQueue(files);
    _queueUpload.push.apply(_queueUpload, Array.from(files));
    if (!_runningUpload) uploadFile();
    e.target.value = "";
  });
});

// File search
$('#search-files').addEventListener('input', function () { renderParsedFiles(); });

// Upload pagination
$('#upload-prev').addEventListener('click', function () {
  if (_uploadPage > 0) { _uploadPage--; renderUploadPage(); }
});
$('#upload-next').addEventListener('click', function () {
  var totalPages = Math.ceil(_uploadItems.length / _uploadPerPage);
  if (_uploadPage < totalPages - 1) { _uploadPage++; renderUploadPage(); }
});

// Sort columns
document.querySelectorAll('th.sortable').forEach(function (th) {
  th.addEventListener('click', function () {
    var col = th.getAttribute('data-sort');
    if (_sortCol === col) _sortDir *= -1;
    else { _sortCol = col; _sortDir = 1; }
    renderParsedFiles();
  });
});

// Select all checkbox
$('#check-all').addEventListener('change', function () {
  var checked = this.checked;
  document.querySelectorAll('.file-check').forEach(function (cb) {
    cb.checked = checked;
    var row = cb.closest('tr');
    var file = row.getAttribute('data-file') || row.getAttribute('data-path');
    if (file) {
      if (checked) { selectedFiles.add(file); row.classList.add('selected'); }
      else { selectedFiles.delete(file); row.classList.remove('selected'); }
    }
  });
  updateMultiBar();
});

// Multi-delete
$('#multi-delete').addEventListener('click', function () { deleteSelected(); });
$('#multi-cancel').addEventListener('click', function () { clearSelection(); });

// Main container clicks
$(".app").addEventListener("click", function (e) {
  // Storage card / browse
  var browseAction = e.target.closest(".act-browse");
  if (browseAction) {
    e.preventDefault();
    var drive = browseAction.getAttribute("data-drive") || currentDrive || "LittleFS";
    var path = browseAction.getAttribute("data-path") || browseAction.closest("tr")?.getAttribute('data-path') || "/";
    if (drive === currentDrive && path === currentPath) return;
    fetchFiles(drive, path);
    return;
  }

  // File checkbox
  var checkbox = e.target.closest('.file-check');
  if (checkbox) {
    var row = checkbox.closest('tr');
    var file = row.getAttribute('data-file') || row.getAttribute('data-path');
    if (file) {
      if (checkbox.checked) { selectedFiles.add(file); row.classList.add('selected'); }
      else { selectedFiles.delete(file); row.classList.remove('selected'); }
      updateMultiBar();
    }
    return;
  }

  // Edit file
  var editFileAction = e.target.closest(".act-edit-file");
  if (editFileAction) {
    e.preventDefault();
    var editor = $(".dialog.editor .file-content");
    var file = editFileAction.closest("tr").getAttribute("data-file");
    if (!file) return;
    $(".dialog.editor .editor-file-name").textContent = file;
    editor.value = "";
    Dialog.loading.show('Loading file...');
    requestGet('/file?fs=' + currentDrive + '&name=' + encodeURIComponent(file) + '&action=edit').then(function (r) {
      editor.value = r;
      editor.setAttribute("data-hash", calcHash(r));
      updateLineNumbers();
      $(".act-save-edit-file").disabled = true;
      var serial = getSerialCommand(file);
      if (serial === undefined) $(".act-run-edit-file").classList.add("hidden");
      else $(".act-run-edit-file").classList.remove("hidden");
      Dialog.loading.hide();
      Dialog.show('editor');
      updateURL(currentDrive, currentPath, file);
    });
    return;
  }

  // One input dialogs (serial, createFile, createFolder, rename)
  var oActionOInput = e.target.closest(".act-oinput");
  if (oActionOInput) {
    e.preventDefault();
    var action = oActionOInput.getAttribute("data-action");
    if (!action) return;
    var value = "", data = "";
    if (action.startsWith("rename")) {
      var row = oActionOInput.closest("tr");
      var filePath = row.getAttribute("data-file") || row.getAttribute("data-path");
      if (filePath) {
        value = filePath.substring(filePath.lastIndexOf("/") + 1);
        data = action + "|" + filePath;
      }
    } else if (action.startsWith("create")) {
      data = action + "|" + currentPath;
    } else {
      data = action;
    }
    Dialog.showOneInput(action, value, data);
    return;
  }

  // Delete single file
  var actDeleteFile = e.target.closest(".act-delete");
  if (actDeleteFile) {
    e.preventDefault();
    var file = actDeleteFile.closest(".file-row").getAttribute("data-file")
      || actDeleteFile.closest(".file-row").getAttribute("data-path");
    if (!file) return;
    if (!confirm("Delete " + file + "?\n\nTHIS CANNOT BE UNDONE!")) return;
    Dialog.loading.show('Deleting...');
    requestGet("/file", { fs: currentDrive, action: 'delete', name: file }).then(function () {
      Dialog.loading.hide();
      Toast.show("Deleted successfully", "success");
      fetchSystemInfo();
      fetchFiles(currentDrive, currentPath);
    });
    return;
  }

  // Play/Run file
  var actPlay = e.target.closest(".act-play");
  if (actPlay) {
    e.preventDefault();
    var cmd = actPlay.getAttribute("data-cmd");
    if (cmd) { actPlay.blur(); runCommand(cmd); }
    return;
  }
});

// Dialog background clicks
$(".dialog-background").addEventListener("click", function (e) {
  if (e.target.matches(".act-dialog-close")) {
    e.preventDefault();
    Dialog.hide();
  }
});

// Save one-input dialog
$(".act-save-oinput-file").addEventListener("click", async function () {
  var dialog = $(".dialog.oinput");
  var fileInput = $("#oinput-input");
  var fileName = fileInput.value.trim();
  if (!fileName) { Toast.show("Name cannot be empty", "error"); return; }
  var action = dialog.getAttribute("data-cache");
  if (!action) return;

  var refreshList = true;
  var parts = action.split("|");
  var actionType = parts[0];
  var path = parts[1];

  if (actionType.startsWith("rename")) {
    Dialog.loading.show('Renaming...');
    await requestPost("/rename", { fs: currentDrive, filePath: path, fileName: fileName });
    Toast.show("Renamed successfully", "success");
  } else if (actionType === "createFolder") {
    Dialog.loading.show('Creating folder...');
    await requestGet("/file?" + new URLSearchParams({
      fs: currentDrive, action: "create", name: path.replace(/\/+$/, '') + '/' + fileName
    }).toString());
    Toast.show("Folder created", "success");
  } else if (actionType === "createFile") {
    Dialog.loading.show('Creating file...');
    await requestGet("/file?" + new URLSearchParams({
      fs: currentDrive, action: "createfile", name: path.replace(/\/+$/, '') + '/' + fileName
    }).toString());
    Toast.show("File created", "success");
  } else if (actionType === "serial") {
    Dialog.loading.show('Running...');
    await runCommand(fileName);
    refreshList = false;
  }

  if (refreshList) fetchFiles(currentDrive, currentPath);
  Dialog.hide();
});

// Credentials
$(".act-save-credential").addEventListener("click", async function () {
  var username = $("#cred-username").value.trim();
  var password = $("#cred-password").value.trim();
  if (!username || !password) { Toast.show("Fields cannot be empty", "error"); return; }
  Dialog.loading.show('Saving...');
  await requestGet("/wifi", { usr: username, pwd: password });
  Dialog.loading.hide();
  Toast.show("Credentials saved!", "success");
});

// Editor save
$(".act-save-edit-file").addEventListener("click", function () { saveEditorFile(); });
$(".act-run-edit-file").addEventListener("click", function () { saveEditorFile(true); this.blur(); });

// Show/hide navigating overlay
var showNavigating = localStorage.getItem('showNavigating') || false;
updateShowHideNavigatingButton();
$(".act-hide-show-navigating").addEventListener("click", function (e) {
  e.preventDefault();
  showNavigating = !showNavigating;
  localStorage.setItem('showNavigating', showNavigating);
  updateShowHideNavigatingButton();
});
function updateShowHideNavigatingButton() {
  $('.act-hide-show-navigating').textContent = "'Navigating...' Overlay: " + (showNavigating ? 'Shown' : 'Hidden');
}

// Reboot
$(".act-reboot").addEventListener("click", async function (e) {
  e.preventDefault();
  if (!confirm("Reboot the device?")) return;
  Dialog.loading.show('Rebooting...');
  await requestGet("/reboot");
  setTimeout(function () { location.reload(); }, 1000);
});

// Navigator pad
$(".navigator-canvas").addEventListener("click", async function (e) {
  var nav = e.target.matches(".nav") ? e.target : e.target.closest(".nav");
  if (!nav) return;
  var direction = nav.getAttribute("data-direction");
  if (direction === "Menu") direction = "Sel 500";
  await runNavigation(direction.toLowerCase());
});

// Keyboard shortcuts
window.addEventListener("keydown", async function (e) {
  var key = e.key.toLowerCase();

  if ($(".dialog.editor:not(.hidden)")) {
    if ((e.ctrlKey || e.metaKey) && key === "s") {
      e.preventDefault(); e.stopImmediatePropagation();
      await saveEditorFile();
    } else if (e.altKey && key === "enter") {
      e.preventDefault(); e.stopImmediatePropagation();
      await saveEditorFile(true);
    }
  }

  if ($(".dialog.navigator:not(.hidden)")) {
    var map = {
      "arrowup": "Up", "arrowdown": "Down", "arrowleft": "Prev",
      "arrowright": "Next", "enter": "Sel", "backspace": "Esc",
      "m": "Menu", "pageup": "NextPage", "pagedown": "PrevPage"
    };
    if (key === 'r') { e.preventDefault(); e.stopImmediatePropagation(); reloadScreen(); return; }
    if (key in map) {
      e.preventDefault(); e.stopImmediatePropagation();
      var navEl = document.querySelector('.navigator-canvas .nav[data-direction="' + map[key] + '"]');
      if (navEl) navEl.click();
      return;
    }
  }

  // Global shortcuts
  if ((e.ctrlKey || e.metaKey) && key === "f" && !$(".dialog:not(.hidden)")) {
    e.preventDefault();
    $('#search-files').focus();
    return;
  }

  if (key === "escape") {
    if ($('#search-files') === document.activeElement) {
      $('#search-files').value = '';
      $('#search-files').blur();
      renderParsedFiles();
      return;
    }
    if (selectedFiles.size > 0) { clearSelection(); return; }
    if ($(".dialog-background.visible")) {
      if ($(".dialog.editor:not(.hidden)")) {
        var editor = $(".dialog.editor .file-content");
        if (isModified(editor)) {
          if (!confirm("Discard unsaved changes?")) return;
        }
      }
      var btnEscape = $(".dialog:not(.hidden) .act-escape");
      if (btnEscape) btnEscape.click();
    }
  }

  if (key === "delete" && selectedFiles.size > 0 && !$(".dialog-background.visible")) {
    e.preventDefault();
    deleteSelected();
  }
});

// Editor text area handlers
$(".file-content").addEventListener("keydown", function (e) {
  if (!$(".dialog.editor:not(.hidden)")) return;
  var textarea = this;
  var start = textarea.selectionStart;
  var end = textarea.selectionEnd;
  var TAB_SIZE = 2;
  var tabSpaces = " ".repeat(TAB_SIZE);
  var leadingSpacesRegex = /^ */;
  var closingCharRegex = /^[\}\)\]]/;

  var insertText = function (text, newStart, newEnd, preserveSelection) {
    textarea.setSelectionRange(start, end);
    document.execCommand("insertText", false, text);
    if (preserveSelection) textarea.setSelectionRange(newStart, newEnd);
    else textarea.setSelectionRange(newStart, newStart);
  };

  var getCurrentLine = function (pos) {
    var lineStart = textarea.value.lastIndexOf("\n", pos - 1) + 1;
    var lineEnd = textarea.value.indexOf("\n", pos);
    var line = textarea.value.slice(lineStart, lineEnd === -1 ? undefined : lineEnd);
    return { line: line, lineStart: lineStart, lineEnd: lineEnd === -1 ? textarea.value.length : lineEnd };
  };

  var handleTab = function (shift) {
    if (start === end) {
      var cl = getCurrentLine(start);
      if (shift) {
        var remove = Math.min(cl.line.match(leadingSpacesRegex)[0].length, TAB_SIZE);
        textarea.setSelectionRange(cl.lineStart, cl.lineEnd);
        document.execCommand("insertText", false, cl.line.slice(remove));
        textarea.setSelectionRange(start - remove, start - remove);
      } else {
        insertText(tabSpaces, start + TAB_SIZE, start + TAB_SIZE, false);
      }
      return;
    }
    var first = getCurrentLine(start);
    var last = getCurrentLine(end === start ? end : end - 1);
    var selectedFullText = textarea.value.slice(first.lineStart, last.lineEnd);
    var fullLines = selectedFullText.split("\n");
    var totalChange = 0;
    var newTextLines = fullLines.map(function (line, idx) {
      if (idx === fullLines.length - 1 && /^\s*$/.test(line)) return line;
      var ls = line.match(leadingSpacesRegex)[0].length;
      if (shift) { var rm = Math.min(ls, TAB_SIZE); totalChange -= rm; return line.slice(rm); }
      else { var add = TAB_SIZE - (ls % TAB_SIZE); totalChange += add; return " ".repeat(add) + line; }
    });
    textarea.setSelectionRange(first.lineStart, last.lineEnd);
    document.execCommand("insertText", false, newTextLines.join("\n"));
    textarea.setSelectionRange(first.lineStart, first.lineStart + newTextLines.join("\n").length);
  };

  var handleEnter = function () {
    var cl = getCurrentLine(start);
    var indent = cl.line.match(leadingSpacesRegex)[0] || "";
    var nextChar = start < textarea.value.length ? textarea.value[start] : "";
    var prevChar = start > 0 ? textarea.value[start - 1] : "";
    var pairs = { "{": "}", "(": ")", "[": "]" };
    if (pairs[prevChar] === nextChar) {
      var extra = " ".repeat(TAB_SIZE);
      var ins = "\n" + indent + extra + "\n" + indent;
      insertText(ins, start + indent.length + extra.length + 1, start + indent.length + extra.length + 1);
    } else {
      var closing = closingCharRegex.test(nextChar) ? "\n" + indent : "";
      insertText("\n" + indent + closing, start + indent.length + 1, start + indent.length + 1);
    }
  };

  var handleAutoPair = function (key) {
    var p = { "(": ")", "{": "}", "[": "]", '"': '"', "'": "'", "`": "`", "<": ">" };
    if (start === end) insertText(key + p[key], start + 1, start + 1, false);
    else {
      var sel = textarea.value.slice(start, end);
      insertText(key + sel + p[key], start + 1, start + 1 + sel.length, true);
    }
  };

  var handleComment = function (cs) {
    var toggleComment = function (line) {
      var ind = line.match(leadingSpacesRegex)[0] || "";
      var content = line.slice(ind.length);
      if (content.startsWith(cs + " ")) return { line: ind + content.slice(cs.length + 1), offset: -(cs.length + 1) };
      if (content.startsWith(cs)) return { line: ind + content.slice(cs.length), offset: -cs.length };
      return { line: ind + cs + " " + content, offset: cs.length + 1 };
    };
    var isCommented = function (line) {
      var c = line.slice((line.match(leadingSpacesRegex)[0] || "").length);
      return c.startsWith(cs + " ") || c.startsWith(cs);
    };
    if (start === end) {
      var cl = getCurrentLine(start);
      var r = toggleComment(cl.line);
      textarea.setSelectionRange(cl.lineStart, cl.lineEnd);
      document.execCommand("insertText", false, r.line);
      textarea.setSelectionRange(start + r.offset, start + r.offset);
      return;
    }
    var first = getCurrentLine(start);
    var last = getCurrentLine(end === start ? end : end - 1);
    var fullLines = textarea.value.slice(first.lineStart, last.lineEnd).split("\n");
    var nonEmpty = fullLines.filter(function (l) { return l.trim().length > 0; });
    var allCommented = nonEmpty.every(isCommented);
    var minInd = Math.min.apply(null, nonEmpty.map(function (l) { return (l.match(leadingSpacesRegex)[0] || "").length; }));
    var newLines = fullLines.map(function (line, idx) {
      if ((idx === fullLines.length - 1 && /^\s*$/.test(line)) || line.trim().length === 0) return line;
      var ind = line.match(leadingSpacesRegex)[0] || "";
      var content = line.slice(ind.length);
      if (allCommented) {
        if (content.startsWith(cs + " ")) return ind + content.slice(cs.length + 1);
        if (content.startsWith(cs)) return ind + content.slice(cs.length);
        return line;
      }
      return " ".repeat(minInd) + cs + " " + line.slice(minInd);
    });
    textarea.setSelectionRange(first.lineStart, last.lineEnd);
    document.execCommand("insertText", false, newLines.join("\n"));
    textarea.setSelectionRange(first.lineStart, first.lineStart + newLines.join("\n").length);
  };

  switch (e.key) {
    case "Tab": e.preventDefault(); handleTab(e.shiftKey); return;
    case "Enter": e.preventDefault(); handleEnter(); return;
    case "/": if (e.ctrlKey || e.metaKey) { e.preventDefault(); handleComment("//"); return; } break;
    case "#": if (e.ctrlKey || e.metaKey) { e.preventDefault(); handleComment("#"); return; } break;
  }

  var nextChar = start < textarea.value.length ? textarea.value[start] : "";
  var closers = [")", "}", "]", ">", '"', "'", "`"];
  if (closers.includes(e.key) && nextChar === e.key) {
    e.preventDefault();
    textarea.setSelectionRange(start + 1, start + 1);
    return;
  }
  var pairs = { "(": ")", "{": "}", "[": "]", '"': '"', "'": "'", "`": "`", "<": ">" };
  if (e.key in pairs) { e.preventDefault(); handleAutoPair(e.key); return; }
});

$(".file-content").addEventListener("keyup", function (e) {
  if ($(".dialog.editor:not(.hidden)")) {
    $(".act-save-edit-file").disabled = !isModified(e.target);
    updateLineNumbers();
  }
});

$(".file-content").addEventListener("scroll", function () {
  if ($(".dialog.editor:not(.hidden)")) syncScrolling();
});

$(".file-content").addEventListener("input", function () {
  if ($(".dialog.editor:not(.hidden)")) updateLineNumbers();
});

document.querySelectorAll(".oinput-text-submit").forEach(function (el) {
  el.addEventListener("keyup", function (e) {
    if (e.key === "Enter" || e.keyCode === 13) {
      e.preventDefault();
      var btn = this.closest(".dialog").querySelector(".btn-default");
      if (btn) btn.click();
    }
  });
});

// Browser back/forward
window.addEventListener('popstate', function (event) {
  if (event.state && event.state.drive && event.state.path) {
    fetchFiles(event.state.drive, event.state.path);
    if (event.state.editFile) restoreEditor(event.state.drive, event.state.editFile);
  } else {
    var p = getURLParams();
    var drive = p.drive || (sdCardAvailable ? "SD" : "LittleFS");
    fetchFiles(drive, p.path || "/");
    if (p.editFile) restoreEditor(drive, p.editFile);
  }
});

async function restoreEditor(drive, editFile) {
  setTimeout(async function () {
    try {
      var editor = $(".dialog.editor .file-content");
      $(".dialog.editor .editor-file-name").textContent = editFile;
      editor.value = "";
      Dialog.loading.show('Loading file...');
      var r = await requestGet('/file?fs=' + drive + '&name=' + encodeURIComponent(editFile) + '&action=edit');
      editor.value = r;
      editor.setAttribute("data-hash", calcHash(r));
      updateLineNumbers();
      $(".act-save-edit-file").disabled = true;
      var serial = getSerialCommand(editFile);
      if (serial === undefined) $(".act-run-edit-file").classList.add("hidden");
      else $(".act-run-edit-file").classList.remove("hidden");
      Dialog.loading.hide();
      Dialog.show('editor');
    } catch (err) {
      updateURL(currentDrive, currentPath, null);
    }
  }, 100);
}

// Init
(async function () {
  await fetchSystemInfo();
  var p = getURLParams();
  var initialDrive = p.drive || (sdCardAvailable ? "SD" : "LittleFS");
  var initialPath = p.path || "/";
  await fetchFiles(initialDrive, initialPath);
  if (p.editFile) restoreEditor(initialDrive, p.editFile);
})();
