"""What the water plugin's ocean materials are made of at their outputs. Run: Tools/py.sh Tools/inspect_drw_material.py 60"""
import unreal
mel = unreal.MaterialEditingLibrary
for name in ["M_DRW_High", "M_DRW_Med", "M_DRW_Far", "M_DRW_Low"]:
    m = unreal.load_asset("/DynamicRealWater/Materials/OceanLOD/" + name)
    if not m:
        unreal.log_warning("DC_RESULT %s missing" % name); continue
    unreal.log_warning("DC_RESULT %s blend=%s shading=%s domain=%s two_sided=%s" % (name, m.get_editor_property("blend_mode"), m.get_editor_property("shading_model"), m.get_editor_property("material_domain"), m.get_editor_property("two_sided")))
    for prop in ["MP_BASE_COLOR", "MP_ROUGHNESS", "MP_SPECULAR", "MP_METALLIC", "MP_EMISSIVE_COLOR", "MP_OPACITY", "MP_NORMAL", "MP_WORLD_POSITION_OFFSET", "MP_SUBSURFACE_COLOR", "MP_REFRACTION"]:
        p = getattr(unreal.MaterialProperty, prop)
        node = mel.get_material_property_input_node(m, p)
        out = mel.get_material_property_input_node_output_name(m, p) if node else ""
        desc = ""
        if node:
            desc = node.get_class().get_name()
            try: desc += " '%s'" % node.get_editor_property("desc")
            except Exception: pass
            try: desc += " fn=%s" % node.get_editor_property("material_function").get_name()
            except Exception: pass
        unreal.log_warning("DC_RESULT   %s <- %s [%s]" % (prop, desc, out))
for name in ["M_DRW_High", "M_DRW_Med"]:
    m = unreal.load_asset("/DynamicRealWater/Materials/OceanLOD/" + name)
    node = mel.get_material_property_input_node(m, unreal.MaterialProperty.MP_BASE_COLOR)
    names = mel.get_material_expression_input_names(node)
    ins = mel.get_inputs_for_material_expression(m, node)
    for n, i in zip(names, ins):
        d = ""
        if i:
            d = i.get_class().get_name()
            for prop in ["parameter_name", "name", "desc"]:
                try: d += " %s=%s" % (prop, i.get_editor_property(prop))
                except Exception: pass
            try: d += " fn=%s" % i.get_editor_property("material_function").get_name()
            except Exception: pass
            d += " out=[%s]" % mel.get_input_node_output_name_for_material_expression(node, i)
        unreal.log_warning("DC_RESULT   %s FoamShading input '%s' <- %s" % (name, n, d))
