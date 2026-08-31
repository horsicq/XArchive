/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XDEARKMODULEREGISTRY_P_H
#define XDEARKMODULEREGISTRY_P_H

struct deark_struct;

// Owns construction of codec contexts and installation of XArchive's selected
// module registry. The upstream all-module factory is intentionally absent.
class XDearkModuleRegistry final
{
public:
    static deark_struct *createContext();

private:
    static void registerSelectedModules(deark_struct *context);

    XDearkModuleRegistry() = delete;
};

#endif  // XDEARKMODULEREGISTRY_P_H
