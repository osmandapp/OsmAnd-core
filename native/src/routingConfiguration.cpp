#ifndef _OSMAND_ROUTING_CONFIGURATION_CPP
#define _OSMAND_ROUTING_CONFIGURATION_CPP
#include "routingConfiguration.h"
#include "generalRouter.h"
#include <expat.h>

class RoutingRulesHandler {
private:
    SHARED_PTR<RoutingConfigurationBuilder> config;
    SHARED_PTR<GeneralRouter> currentRouter;
    string preType;
    vector<RoutingRule*> rulesStack;
    RouteDataObjectAttribute currentAttribute;
    
public:
    RoutingRulesHandler(SHARED_PTR<RoutingConfigurationBuilder> routingConfig) : config(routingConfig) {
    }
    
private:
    static UNORDERED(map)<string, string>& parseAttributes(const char **atts, UNORDERED(map)<string, string>& m) {
        while (*atts != NULL) {
            m[string(atts[0])] = string(atts[1]);
            atts += 2;
        }
        return m;
    }
    
    static string attrValue(UNORDERED(map)<string, string>& m, const string& key, string def = "") {
        if (m.find(key) != m.end() && m[key] != "") {
            return m[key];
        }
        return def;
    }
    
    static SHARED_PTR<GeneralRouter> parseRoutingProfile(UNORDERED(map)<string, string>& attrsMap, SHARED_PTR<RoutingConfigurationBuilder> config) {
        string currentSelectedRouter = attrValue(attrsMap, "name");
        UNORDERED(map)<string, string> attrs;
        attrs.insert(attrsMap.begin(), attrsMap.end());
        GeneralRouterProfile c = parseGeneralRouterProfile(attrValue(attrsMap, "baseProfile"), GeneralRouterProfile::CAR);
        SHARED_PTR<GeneralRouter> currentRouter = SHARED_PTR<GeneralRouter>(new GeneralRouter(c, attrs));
        config->addRouter(currentSelectedRouter, currentRouter);
        return currentRouter;
    }
    
    static void parseAttribute(UNORDERED(map)<string, string>& attrsMap, SHARED_PTR<RoutingConfigurationBuilder> config, SHARED_PTR<GeneralRouter> currentRouter) {
        if (currentRouter != nullptr) {
            currentRouter->addAttribute(attrValue(attrsMap, "name"), attrValue(attrsMap, "value"));
        } else {
            config->addAttribute(attrValue(attrsMap, "name"), attrValue(attrsMap, "value"));
        }
    }
    
    static void parseRoutingParameter(UNORDERED(map)<string, string>& attrsMap, SHARED_PTR<GeneralRouter> currentRouter) {
        string description = attrValue(attrsMap, "description");
        string group = attrValue(attrsMap, "group");
        string name = attrValue(attrsMap, "name");
        string id = attrValue(attrsMap, "id");
        string type = attrValue(attrsMap, "type");
        bool defaultValue = parseBool(attrValue(attrsMap, "default"), false);
        if ("boolean" == to_lowercase(type)) {
            currentRouter->registerBooleanParameter(id, group, name, description, defaultValue);
        } else if ("numeric" == to_lowercase(type)) {
            string values = attrValue(attrsMap, "values");
            string valueDescriptions = attrValue(attrsMap, "valueDescriptions");
            vector<string> strValues = split_string(values, ",");
            vector<double> vls;
            for (int i = 0; i < strValues.size(); i++) {
                vls.push_back(parseFloat(strValues[i], false));
            }
            currentRouter->registerNumericParameter(id, name, description, vls, split_string(valueDescriptions, ","));
        }
    }
    
    static bool checkTag(const string& pname) {
        return "select" == pname || "if" == pname || "ifnot" == pname
        || "gt" == pname || "le" == pname || "eq" == pname;
    }
    
    static void addSubclause(RoutingRule* rr, RouteAttributeContext& ctx, SHARED_PTR<GeneralRouter> currentRouter) {
        bool no = "ifnot" == rr->tagName;
        if (rr->param != "") {
            ctx.getLastRule()->registerAndParamCondition(rr->param, no);
        }
        if (rr->t != "") {
            ctx.getLastRule()->registerAndTagValueCondition(currentRouter.get(), rr->t, rr->v, no);
        }
        if (rr->tagName == "gt") {
            ctx.getLastRule()->registerGreatCondition(rr->value1, rr->value2, rr->type);
        } else if (rr->tagName == "le") {
            ctx.getLastRule()->registerLessCondition(rr->value1, rr->value2, rr->type);
        } else if (rr->tagName == "eq") {
            ctx.getLastRule()->registerEqualCondition(rr->value1, rr->value2, rr->type);
        }
    }
    
    static void parseRoutingRule(const string& pname, UNORDERED(map)<string, string>& attrsMap, SHARED_PTR<GeneralRouter> currentRouter, RouteDataObjectAttribute& attr, string parentType, vector<RoutingRule*>& stack) {
        if (checkTag(pname)) {
            RoutingRule* rr = new RoutingRule();
            rr->tagName = pname;
            rr->t = attrValue(attrsMap, "t");
            rr->v = attrValue(attrsMap, "v");
            rr->param = attrValue(attrsMap, "param");
            rr->value1 = attrValue(attrsMap, "value1");
            rr->value2 = attrValue(attrsMap, "value2");
            rr->type = attrValue(attrsMap, "type");
            if ((rr->type.length() == 0) && parentType.length() > 0) {
                rr->type = parentType;
            }
            
            RouteAttributeContext& ctx = currentRouter->getObjContext(attr);
            if ("select" == rr->tagName) {
                string val = attrValue(attrsMap, "value");
                string type = rr->type;
                RouteAttributeEvalRule* rule = ctx.newEvaluationRule();
                rule->registerSelectValue(val, type);
                addSubclause(rr, ctx, currentRouter);
                for (int i = 0; i < stack.size(); i++) {
                    addSubclause(stack[i], ctx, currentRouter);
                }
            } else if (stack.size() > 0 && stack.back()->tagName == "select") {
                addSubclause(rr, ctx, currentRouter);
            }
            stack.push_back(rr);
        }
    }
    
public:
    static void startElementHandler(void *data, const char *tag, const char **atts) {
        RoutingRulesHandler* handler = (RoutingRulesHandler*) data;
        string name(tag);
        UNORDERED(map)<string, string> attrsMap;
        parseAttributes(atts, attrsMap);
        
        if ("osmand_routing_config" == name) {
            handler->config->defaultRouter = attrValue(attrsMap, "defaultProfile", "");
        } else if ("routingProfile" == name) {
            handler->currentRouter = parseRoutingProfile(attrsMap, handler->config);
        } else if ("attribute" == name) {
            parseAttribute(attrsMap, handler->config, handler->currentRouter);
        } else if ("parameter" == name) {
            parseRoutingParameter(attrsMap, handler->currentRouter);
        } else if ("point" == name || "way" == name) {
            string attribute = attrValue(attrsMap, "attribute");
            handler->currentAttribute = parseRouteDataObjectAttribute(attribute, RouteDataObjectAttribute::UNDEFINED);
            handler->preType = attrValue(attrsMap, "type");
        } else {
            parseRoutingRule(name, attrsMap, handler->currentRouter, handler->currentAttribute, handler->preType, handler->rulesStack);
        }
        
    }
    
    static void endElementHandler(void *data, const char *tag) {
        RoutingRulesHandler* handler = (RoutingRulesHandler*) data;
        string pname(tag);
        if (checkTag(pname)) {
            RoutingRule* rr = handler->rulesStack.back();
            handler->rulesStack.pop_back();
            delete rr;
        }
    }
};

SHARED_PTR<RoutingConfigurationBuilder> parseRoutingConfigurationFromXml(const char* filename) {
    
    XML_Parser parser = XML_ParserCreate(NULL);
    SHARED_PTR<RoutingConfigurationBuilder> config = SHARED_PTR<RoutingConfigurationBuilder>(new RoutingConfigurationBuilder());
    RoutingRulesHandler* handler = new RoutingRulesHandler(config);
    XML_SetUserData(parser, handler);
    XML_SetElementHandler(parser, RoutingRulesHandler::startElementHandler, RoutingRulesHandler::endElementHandler);
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "File can not be open %s", filename);
        XML_ParserFree(parser);
        delete handler;
        return nullptr;
    }
    char buffer[512];
    bool done = false;
    while (!done) {
        fgets(buffer, sizeof(buffer), file);
        int len = (int)strlen(buffer);
        if (feof(file) != 0) {
            done = true;
        }
        if (XML_Parse(parser, buffer, len, done) == XML_STATUS_ERROR) {
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Routing xml parsing error: %s at line %d\n",
                              XML_ErrorString(XML_GetErrorCode(parser)), (int)XML_GetCurrentLineNumber(parser));
            fclose(file);
            XML_ParserFree(parser);
            delete handler;
            return nullptr;
        }
    }
    XML_ParserFree(parser);
    delete handler;
    fclose(file);
    
    return config;
}

#endif /*_OSMAND_ROUTING_CONFIGURATION_CPP*/
