// This file is part of Deark.
// Copyright (C) 2016 Jason Summers
// See xdearkdecoder.LICENSE for terms of use.

#define DE_NOT_IN_MODULE
#include "xdearkengine_config_p.h"
#include "xdearkengine_private_p.h"
#include "xdearkengine_api_p.h"
#include "xdearkmoduleregistry_p.h"

#define DE_MODULE(x)      DE_DECLARE_MODULE(x);
#define DE_MODULE_LAST(x) DE_DECLARE_MODULE(x);
#include "xdearkengine_modules_p.h"
#undef DE_MODULE
#undef DE_MODULE_LAST

namespace {

class XDearkModule final
{
public:
    explicit XDearkModule(de_module_getinfo_fn registerFunction)
        : m_registerFunction(registerFunction)
    {
    }

    void install(deark *context) const
    {
        m_registerFunction(
            context, &context->module_info[context->num_modules++]);
    }

private:
    de_module_getinfo_fn m_registerFunction;
};

static void disable_module(deark *c, struct deark_module_info *mi)
{
	mi->identify_fn = NULL;
	mi->run_fn = NULL;
}

// Caller supplies mod_set[c->num_modules].
// Sets mod_set[n] = 1 for each module in 's', a comma-separated list.
// Does not modify other entries in mod_set[].
static void module_list_string_to_set(deark *c, const char *s, u8 *mod_set)
{
	char tmpname[80];
	const char *ptr1, *ptr2;

	if(!s) return;

	ptr1 = s;
	while(1) {
		int idx;

		tmpname[0] = '\0';
		ptr2 = de_strchr(ptr1, ',');
		if(ptr2) {
			size_t len;
			len = (size_t)(ptr2-ptr1);
			if(len<sizeof(tmpname)) {
				de_memcpy(tmpname, ptr1, len);
				tmpname[len] = '\0';
			}
			ptr1 = ptr2 + 1;
		}
		else {
			de_strlcpy(tmpname, ptr1, sizeof(tmpname));
		}

		idx = de_get_module_idx_by_id(c, tmpname);
		if(idx>=0) {
			mod_set[idx] = 1;
		}

		if(!ptr2) break;
	}
}

static void disable_modules_as_requested(deark *c)
{
	int k;
	u8 *mod_set;

	if(!c->onlymods_string && !c->disablemods_string &&
		!c->onlydetectmods_string && !c->nodetectmods_string)
	{
		return;
	}

	mod_set = de_malloc(c, c->num_modules);

	if(c->onlymods_string) {
		module_list_string_to_set(c, c->onlymods_string, mod_set);
		// Disable modules not in the list
		for(k=0; k<c->num_modules; k++) {
			if(!mod_set[k]) {
				disable_module(c, &c->module_info[k]);
			}
		}
	}

	if(c->disablemods_string) {
		de_zeromem(mod_set, c->num_modules);
		module_list_string_to_set(c, c->disablemods_string, mod_set);
		// Disable modules in the list
		for(k=0; k<c->num_modules; k++) {
			if(mod_set[k]) {
				disable_module(c, &c->module_info[k]);
			}
		}
	}

	if(c->onlydetectmods_string) {
		de_zeromem(mod_set, c->num_modules);
		module_list_string_to_set(c, c->onlydetectmods_string, mod_set);
		// Set MODFLAG_DISABLEDETECT for modules not in the list
		for(k=0; k<c->num_modules; k++) {
			if(!mod_set[k]) {
				c->module_info[k].flags |= DE_MODFLAG_DISABLEDETECT;
			}
		}
	}

	if(c->nodetectmods_string) {
		de_zeromem(mod_set, c->num_modules);
		module_list_string_to_set(c, c->nodetectmods_string, mod_set);
		// Set MODFLAG_DISABLEDETECT for modules in the list
		for(k=0; k<c->num_modules; k++) {
			if(mod_set[k]) {
				c->module_info[k].flags |= DE_MODFLAG_DISABLEDETECT;
			}
		}
	}

	de_free(c, mod_set);
}

}  // namespace

void XDearkModuleRegistry::registerSelectedModules(deark *c)
{
	static const XDearkModule modules[] = {
#define DE_MODULE(x)      XDearkModule(&x),
#define DE_MODULE_LAST(x) XDearkModule(&x)
#include "xdearkengine_modules_p.h"
#undef DE_MODULE
#undef DE_MODULE_LAST
	};
	size_t num_modules;

	if(c->module_info) return; // Already done.

	num_modules = DE_ARRAYCOUNT(modules);

	c->module_info = de_mallocarray(c, num_modules, sizeof(struct deark_module_info));

	for(const XDearkModule &module : modules) {
		module.install(c);
	}

	disable_modules_as_requested(c);
}

deark *XDearkModuleRegistry::createContext()
{
	deark *c;
	c = de_create_internal();
	c->module_register_fn = &XDearkModuleRegistry::registerSelectedModules;
	return c;
}
