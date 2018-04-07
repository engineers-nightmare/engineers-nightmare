#!/usr/bin/env python3

#
# From the root of the EN tree, run:
#    gen/gen_enums.py
#
# (Or, on windows: python gen/gen_enums.py)
#

import re
import collections
import sys
import os
import glob

def fail(args):
    print(args)
    sys.exit(1)

def main():
    files = [f for f in glob.glob('gen/enum/*')]

    pattern = re.compile(r'\s*(\w+)\s*(?:=\s*(\d+))?\s*(?:,\s*"([^"]*)")?\s*')
    tab = "    "

    output_dir = "src/enums/"

    do_not_modify = "/* THIS FILE IS AUTOGENERATED BY gen/gen_enums.py; DO NOT HAND-MODIFY */\n"

    print("Genning enums")
    header_file = os.path.join(output_dir, "enums.h")
    source_file = os.path.join(output_dir, "enums.cc")

    with open(header_file, "w") as header:
        with open(source_file, "w") as source:
            header.write("#pragma once\n")
            header.write("\n")
            header.write("#include <libconfig.h>\n")
            header.write("#include \"../libconfig_shim.h\"\n")
            header.write("\n")
            header.write(do_not_modify)
            header.write("\n")
            header.write("\n")
            header.write("template<typename T>\n")
            header.write(tab + "T get_enum(const char *e);\n\n")

            source.write(do_not_modify)
            source.write("\n")
            source.write("#include \"enums.h\"\n")
            source.write("\n")
            source.write("#include <cassert>\n")
            source.write("#include <cstring>\n")
            source.write("\n")

            for fl in files:
                print("  %s" % fl)
                enum_name = fl.split(os.sep)[-1]
                input_file = fl

                next_value = 0
                desc = ""

                fields = collections.OrderedDict()
                descs = collections.OrderedDict()

                for line in open(input_file, 'r'):
                    line = line.strip()

                    if len(line) == 0:
                        continue

                    match = pattern.match(line)

                    if match is None:
                        fail("Syntax error: [" + line + "]")

                    field_name = match.group(1)

                    if field_name in fields:
                        fail("Double field name: " + field_name)

                    value_string = match.group(2)

                    desc_string = match.group(3)
                    if desc_string is not None:
                        desc = desc_string
                    else:
                        desc = enum_name

                    if value_string is not None:
                        value = int(value_string)

                        if value < next_value:
                            fail("Not a monotonic progression from " + next_value + " to " + value + " for enum field " + field_name)

                        next_value = value

                    fields[field_name] = next_value
                    descs[field_name] = desc

                    next_value += 1

                header.write("\n")
                header.write("// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n")
                header.write("enum class " + enum_name + "\n")
                header.write("{\n")
                for field_name, fieldValue in fields.items():
                    header.write(tab + "%s = %s,\n" % (field_name, str(fieldValue)))
                header.write(tab + "invalid = -1,\n")
                header.write("};\n")
                header.write("\n")
                header.write("const char* get_enum_description(%s value);\n" % enum_name)
                header.write("\n")
                header.write("const char* get_enum_string(%s value);\n" % enum_name)
                header.write("\n")
                header.write("template<> %s get_enum<%s>(const char *e);\n" % (enum_name, enum_name))
                header.write("\n")
                header.write("%s config_setting_get_%s(const config_setting_t *setting);\n" % (enum_name, enum_name))
                header.write("\n")
                header.write("int config_setting_set_%s(config_setting_t *setting, %s value);\n" % (enum_name, enum_name))
                header.write("\n")
                header.write("int config_setting_lookup_%s(const config_setting_t *setting, const char *name, %s *value);\n" % (enum_name, enum_name))


                source.write("\n")
                source.write("// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n")
                source.write("const char* get_enum_description(%s value) {\n" % enum_name)
                source.write(tab + "switch(value)\n")
                source.write(tab + "{\n")
                for field_name in fields.keys():
                    source.write(tab + "case %s::%s:\n" % (enum_name, field_name))
                    source.write(tab + tab + "return \"%s\";\n" % descs[field_name])
                source.write(tab + "default:\n")
                source.write(tab + tab + "assert(false);\n")
                source.write(tab + tab + "return nullptr;\n")
                source.write(tab + "}\n")
                source.write("}\n")
                source.write("\n")
                source.write("const char* get_enum_string(%s value) {\n" % enum_name)
                source.write(tab + "switch(value)\n")
                source.write(tab + "{\n")
                for field_name in fields.keys():
                    source.write(tab + "case %s::%s:\n" % (enum_name, field_name))
                    source.write(tab + tab + "return \"%s\";\n" % field_name)
                source.write(tab + "default:\n")
                source.write(tab + tab + "assert(false);\n")
                source.write(tab + tab + "return nullptr;\n")
                source.write(tab + "}\n")
                source.write("}\n")
                source.write("\n")
                source.write("template<> %s get_enum<%s>(const char *e) {\n" % (enum_name, enum_name))
                source.write(tab + "auto val{%s::invalid};\n" % enum_name)
                for field_name in fields.keys():
                    source.write(tab + "if (!strcmp(e, \"%s\")) {\n" % field_name)
                    source.write(tab + tab + "val = %s::%s;\n" % (enum_name, field_name))
                    source.write(tab + "}\n")
                source.write(tab + "assert(val != %s::invalid);\n" % enum_name)
                source.write(tab + "return val;\n")
                source.write("}\n")
                source.write("\n")
                source.write("%s config_setting_get_%s(const config_setting_t *setting) {\n" % (enum_name, enum_name))
                source.write(tab + "const char *str = config_setting_get_string(setting);\n")
                source.write(tab + "return get_enum<%s>(str);\n" % enum_name)
                source.write("}\n")
                source.write("\n")
                source.write("int config_setting_set_%s(config_setting_t *setting, %s value) {\n" % (enum_name, enum_name))
                source.write(tab + "auto str = get_enum_string(value);\n")
                source.write(tab + "return (config_setting_set_string(setting, str));\n")
                source.write("}\n")
                source.write("\n")
                source.write("int config_setting_lookup_%s(const config_setting_t *setting, const char *name, %s *value) {\n" % (enum_name, enum_name))
                source.write(tab + "auto *member = config_setting_get_member(setting, name);\n")
                source.write(tab + "if(!member) {\n")
                source.write(tab + tab + "return CONFIG_FALSE;\n")
                source.write(tab + "}\n")
                source.write("\n")
                source.write(tab + "*value = (%s)config_setting_get_%s(member);\n" % (enum_name, enum_name))
                source.write(tab + "return CONFIG_TRUE;\n")
                source.write("}\n")

    return 0

if __name__ == '__main__':
    sys.exit(main())
