#include "universal_history.h"

std::vector<HistEntry> hist_load(FS &fs, const String &file) {
    std::vector<HistEntry> list;
    File f = fs.open(file, FILE_READ);
    if (!f) return list;
    while (f.available()) {
        String line = f.readStringUntil('\n');
        if (line.endsWith("\r")) line.remove(line.length() - 1);
        line.trim();
        if (line.length() == 0) continue;
        // Format: <D|F><TAB>path<TAB>name
        HistEntry e;
        int t1 = line.indexOf('\t');
        if (t1 <= 0) continue;
        e.isDir = (line.charAt(0) == 'D');
        int t2 = line.indexOf('\t', t1 + 1);
        e.path = line.substring(t1 + 1, (t2 == -1) ? line.length() : t2);
        if (t2 != -1) e.name = line.substring(t2 + 1);
        e.path.trim();
        e.name.trim();
        if (e.path.length() == 0) continue;
        list.push_back(e);
    }
    f.close();
    return list;
}

bool hist_save(FS &fs, const String &file, const std::vector<HistEntry> &list) {
    File f = fs.open(file, FILE_WRITE);
    if (!f) return false;
    for (const auto &e : list) {
        f.print(e.isDir ? 'D' : 'F');
        f.print('\t');
        f.print(e.path);
        f.print('\t');
        f.println(e.name);
    }
    f.close();
    return true;
}

bool hist_has(const std::vector<HistEntry> &list, const String &path, const String &name) {
    for (const auto &e : list) {
        if (e.path == path && e.name == name) return true;
    }
    return false;
}

void hist_add(
    std::vector<HistEntry> &list, const String &path, const String &name, bool isDir, size_t cap
) {
    hist_remove(list, path, name);
    HistEntry e;
    e.path = path;
    e.name = name;
    e.isDir = isDir;
    list.insert(list.begin(), e);
    while (list.size() > cap) list.pop_back();
}

void hist_remove(std::vector<HistEntry> &list, const String &path, const String &name) {
    for (size_t i = 0; i < list.size();) {
        if (list[i].path == path && list[i].name == name) list.erase(list.begin() + i);
        else i++;
    }
}
