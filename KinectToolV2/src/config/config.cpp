#include "../include/globals.h"

// =====================================================================
// Configuration — defaults / save / load
// =====================================================================

static int FindJointByKey(const char* key)
{
    for (int j = 0; j < JointType_Count; ++j)
        if (strcmp(g_JointKeys[j], key) == 0) return j;
    return -1;
}

void ConfigSetDefaults()
{
    memset(g_config.autoJoints, 0, sizeof(g_config.autoJoints));
    for (int i = 0; i < BODY_COUNT; ++i) {
        g_config.persons[i].enabled = true;
        memset(g_config.persons[i].joints, 0, sizeof(g_config.persons[i].joints));
    }
    g_config.globalInputEnabled = false;
    
    for (int j = 0; j < JointType_Count; ++j) {
        g_config.autoMappings[j].items.clear();
        for (int i = 0; i < BODY_COUNT; ++i) {
            g_config.persons[i].mappings[j].items.clear();
        }
    }
}

void WriteMappings(FILE* f, JointMapping* mappings) {
    fprintf(f, "\"mappings\": [");
    bool first = true;
    for (int j = 0; j < JointType_Count; ++j) {
        for (const auto& item : mappings[j].items) {
            fprintf(f, "%s{\"joint\": \"%s\", \"key\": %d, \"dir\": %d, \"thresh\": %f, \"behavior\": %d}",
                first ? "" : ", ", g_JointKeys[j], item.keyCode, (int)item.direction, item.threshold, (int)item.behavior);
            first = false;
        }
    }
    fprintf(f, "]");
}

void ConfigSave()
{
    FILE* f = nullptr;
    _wfopen_s(&f, g_configPath, L"w");
    if (!f) return;

    fprintf(f, "{\n  \"globalInputEnabled\": %s,\n  \"auto\": {\n    \"joints\": [", g_config.globalInputEnabled ? "true" : "false");
    bool first = true;
    for (int j = 0; j < JointType_Count; ++j)
    {
        if (g_config.autoJoints[j])
        {
            fprintf(f, "%s\"%s\"", first ? "" : ", ", g_JointKeys[j]);
            first = false;
        }
    }
    fprintf(f, "],\n    ");
    WriteMappings(f, g_config.autoMappings);
    fprintf(f, "\n  },\n  \"persons\": [\n");

    for (int i = 0; i < BODY_COUNT; ++i)
    {
        fprintf(f, "    {\"enabled\": %s, \"joints\": [",
            g_config.persons[i].enabled ? "true" : "false");
        first = true;
        for (int j = 0; j < JointType_Count; ++j)
        {
            if (g_config.persons[i].joints[j])
            {
                fprintf(f, "%s\"%s\"", first ? "" : ", ", g_JointKeys[j]);
                first = false;
            }
        }
        fprintf(f, "], ");
        WriteMappings(f, g_config.persons[i].mappings);
        fprintf(f, "}%s\n", (i < BODY_COUNT - 1) ? "," : "");
    }
    fprintf(f, "  ]\n}\n");
    fclose(f);
}

// Minimal JSON parser — only handles our own output format
void ParseJointsFromPos(const char* p, bool* joints)
{
    // Find '[' then extract quoted strings until ']'
    while (*p && *p != '[') p++;
    if (!*p) return;
    p++; // skip '['

    while (*p && *p != ']')
    {
        // Skip to next quote
        while (*p && *p != '"' && *p != ']') p++;
        if (*p != '"') break;
        p++; // skip opening quote

        char key[64] = {};
        int k = 0;
        while (*p && *p != '"' && k < 63) key[k++] = *p++;
        key[k] = 0;
        if (*p == '"') p++;

        int idx = FindJointByKey(key);
        if (idx >= 0) joints[idx] = true;
    }
}

void ParseMappingsFromPos(const char* p, JointMapping* mappings)
{
    while (*p && *p != '[') p++;
    if (!*p) return;
    p++;

    while (*p && *p != ']')
    {
        p = strchr(p, '{');
        if (!p) break;
        
        const char* end = strchr(p, '}');
        if (!end) break;
        const char* endBracket = strchr(p, ']');
        if (endBracket && end > endBracket) break;

        char jointName[64] = {};
        const char* jKey = strstr(p, "\"joint\"");
        if (jKey && jKey < end) {
            jKey = strchr(jKey + 7, '"');
            if (jKey) {
                jKey++;
                int k = 0;
                while (*jKey && *jKey != '"' && k < 63) jointName[k++] = *jKey++;
            }
        }
        
        int jIdx = FindJointByKey(jointName);
        if (jIdx >= 0) {
            SingleMapping sm;
            const char* kKey = strstr(p, "\"key\"");
            if (kKey && kKey < end) sm.keyCode = atoi(kKey + 6);
            
            const char* dKey = strstr(p, "\"dir\"");
            if (dKey && dKey < end) sm.direction = (GestureDir)atoi(dKey + 6);
            
            const char* tKey = strstr(p, "\"thresh\"");
            if (tKey && tKey < end) sm.threshold = (float)atof(tKey + 9);
            
            const char* bKey = strstr(p, "\"behavior\"");
            if (bKey && bKey < end) sm.behavior = (GestureBehavior)atoi(bKey + 11);
            
            mappings[jIdx].items.push_back(sm);
        }
        p = end + 1;
    }
}

void ConfigLoad()
{
    ConfigSetDefaults();

    FILE* f = nullptr;
    _wfopen_s(&f, g_configPath, L"r");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return; }

    char* buf = new char[sz + 1];
    size_t rd = fread(buf, 1, sz, f);
    buf[rd] = 0;
    fclose(f);

    const char* gbl = strstr(buf, "\"globalInputEnabled\"");
    if (gbl) {
        gbl += 20;
        while (*gbl && (*gbl == ' ' || *gbl == ':')) gbl++;
        g_config.globalInputEnabled = (strncmp(gbl, "true", 4) == 0);
    }

    // Parse "auto" section
    const char* autoSec = strstr(buf, "\"auto\"");
    if (autoSec)
    {
        const char* jointsKey = strstr(autoSec, "\"joints\"");
        if (jointsKey)
            ParseJointsFromPos(jointsKey, g_config.autoJoints);
            
        const char* mapKey = strstr(autoSec, "\"mappings\"");
        if (mapKey)
            ParseMappingsFromPos(mapKey, g_config.autoMappings);
    }

    // Parse "persons" array
    const char* personsSec = strstr(buf, "\"persons\"");
    if (personsSec)
    {
        const char* p = personsSec;
        for (int i = 0; i < BODY_COUNT; ++i)
        {
            // Find next '{' for this person
            p = strchr(p, '{');
            if (!p) break;

            // Find closing '}' to bound this person block
            const char* blockEnd = strchr(p, '}');
            if (!blockEnd) break;

            // Parse "enabled"
            const char* en = strstr(p, "\"enabled\"");
            if (en && en < blockEnd)
            {
                en += 9; // skip "enabled"
                while (*en && (*en == ' ' || *en == ':' || *en == '\t')) en++;
                g_config.persons[i].enabled = (strncmp(en, "true", 4) == 0);
            }

            // Parse "joints"
            const char* jk = strstr(p, "\"joints\"");
            if (jk && jk < blockEnd)
                ParseJointsFromPos(jk, g_config.persons[i].joints);
                
            // Parse "mappings"
            const char* mk = strstr(p, "\"mappings\"");
            if (mk && mk < blockEnd)
                ParseMappingsFromPos(mk, g_config.persons[i].mappings);

            p = blockEnd + 1;
        }
    }

    delete[] buf;
}

