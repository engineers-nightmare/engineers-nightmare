import glob
import os
import sys

from mako.template import Template


def find_matching_stub(body_field, stubs):
    for x in stubs:
        if x['name'] == body_field['name']:
            return x
    return None


def main():
    files = [f for f in glob.glob('gen/comp/*')]
    comp_names = sorted([os.path.basename(f) for f in files])

    component_managers_header_template = Template(filename='gen/templates/component_managers.h.gen')
    component_header_template = Template(filename='gen/templates/comp_component.h.gen')
    component_impl_template = Template(filename='gen/templates/comp_component.cc.gen')

    print("Genning header: src/component/component_managers.h")
    with open("src/component/component_managers.h", "w") as g:
        g.write(component_managers_header_template.render(comp_names=comp_names))

    for comp_file in files:
        print("Genning from %s to " % comp_file)
        stub_fields = []
        body_fields = []
        dependencies = []
        component_name = os.path.basename(comp_file)
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
                for body in body_fields:
                    body['stub'] = find_matching_stub(body, stub_fields)
        opts = {'comp_name': component_name, 'stubs': stub_fields, 'bodies': body_fields, 'dependencies': dependencies}

        print("  header: src/component/%s_component.h" % component_name)
        with open("src/component/%s_component.h" % component_name, "w") as g:
            g.write(component_header_template.render(**opts))

        print("  source: src/component/%s_component.cc" % component_name)
        with open("src/component/%s_component.cc" % component_name, "w") as g:
            g.write(component_impl_template.render(**opts))

    return 0


if __name__ == '__main__':
    sys.exit(main())
