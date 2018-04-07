#!/usr/bin/env python3

import glob
import os
import sys

from mako.template import Template
from mako import exceptions


def find_matching_stub(body_field, stubs):
    for x in stubs:
        if x['name'] == body_field['name']:
            return x
    return None


class component:
    def __init__(self, cname, cfile, uiname, sfields, bfields, depends):
        self.comp_name = cname
        self.comp_file = cfile
        self.ui_name = uiname
        self.stub_fields = sfields
        self.body_fields = bfields
        self.dependencies = depends


def load_component(comp_file):
    stub_fields = []
    body_fields = []
    dependencies = []
    component_name = os.path.basename(comp_file)
    ui_name = "Invalid Name"
    with open(comp_file, 'r') as f:
        prev = 'entity'
        for l in f:
            parts = l.strip().split(',')
            if parts[0] == 'body':
                body_fields.append({'type': parts[1], 'name': parts[2], 'default': parts[3], 'prev': prev,
                                    'comp_name': component_name})
                prev = parts[2]
            elif parts[0] == 'stub':
                stub_fields.append(
                    {'otype': parts[1], 'type': parts[2], 'pre': parts[3], 'name': parts[4], 'extra': parts[5],
                     'comp_name': component_name})
            elif parts[0] == 'depends':
                dependencies = parts[1:]
            elif parts[0] == 'ui_name':
                ui_name = parts[1]
            for body in body_fields:
                body['stub'] = find_matching_stub(body, stub_fields)

    comp = component(component_name, comp_file, ui_name, stub_fields, body_fields, dependencies)
    return comp


def main():
    files = [f for f in glob.glob('gen/comp/*')]
    comps = {f.split('/')[2]: load_component(f) for f in files}

    try:
        gen_component_managers_header(comps)
        gen_individual_components(comps)
    except:
        print(exceptions.text_error_template().render())

    return 0


def gen_component_managers_header(comps):
    print("Genning header: src/component/component_managers.h")
    component_managers_header_template = Template(filename='gen/templates/component_managers.h.gen')
    with open("src/component/component_managers.h", "w") as g:
        g.write(component_managers_header_template.render(comps=sorted(comps)))


def gen_individual_components(comps):
    component_header_template = Template(filename='gen/templates/comp_component.h.gen')
    component_impl_template = Template(filename='gen/templates/comp_component.cc.gen')

    for comp_name in sorted(comps.keys()):
        print("Genning from %s to " % comp_name)
        comp = comps[comp_name]

        print("  header: src/component/%s_component.h" % comp_name)
        with open("src/component/%s_component.h" % comp_name, "w") as g:
            g.write(component_header_template.render(comp_name=comp_name, comp=comp))

        print("  source: src/component/%s_component.cc" % comp_name)
        with open("src/component/%s_component.cc" % comp_name, "w") as g:
            g.write(component_impl_template.render(comp_name=comp_name, comp=comp))



if __name__ == '__main__':
    sys.exit(main())
