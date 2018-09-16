from __future__ import print_function
import json
import os.path
import shutil
import sys
import tarfile
import tempfile

# This script converts a Version 1 MoonQuest Sprite Project (*.tar) into a Version 2 file (*.mqs)
next_id = 0
def get_next_id():
    global next_id
    next_id += 1
    return next_id


def process(filename, outputfilename):
     
    
    tar_output_dir = tempfile.mkdtemp()

    with tarfile.open(filename, "r") as tar:        
        print("Extracting tar to: %s" % tar_output_dir)
        tar.extractall(path=tar_output_dir, members=None)
        data_json_path = os.path.join(tar_output_dir, "data.json")
        data_obj = json.load(open(data_json_path))

    assert data_obj is not None

    print("Found data.json")

    assert data_obj["version"] == 1, "File version != 1"
    print("Upgrading version from %s to 2" % (str(data_obj["version"])))
    data_obj["version"] = 2

    # add folders info
    folders = { "children": {} }
    
    def create_dirs(dir_list):
        ''' Create all the directories in list recursively '''
        ''' Return the leaf dir '''
        current_dir = folders
        for dirname in dir_list:
            if dirname in current_dir["children"]:
                current_dir = current_dir["children"][dirname]
            else:                
                sub_dir = {
                    "name": dirname,
                    "id": get_next_id(),
                    "children": {}
                }
                if current_dir != folders:
                    sub_dir["parent"] = current_dir["id"]                
                current_dir["children"][dirname] = sub_dir                
                current_dir = sub_dir
        return current_dir

    parts = data_obj["parts"]
    part_list = []
    for name, part in parts.iteritems():
        part["id"] = get_next_id()
        names = list(name.split("/"))        
        part_dir = None
        if len(names) > 1:
            part_dir = create_dirs(names[:-1])
        part["name"] = names[-1]
        if part_dir:
            part["parent"] = part_dir["id"]
        part_list.append(part)
    data_obj["parts"] = part_list

    print("WARNING: Deleting comps while debugging!")
    del data_obj["comps"]

    # build folders tree
    def flatten(node):
        lst = []
        for k, v in node["children"].iteritems():
            lst.extend(flatten(v))
        if "name" in node:
            del node["children"] 
            lst.append(node)
        return lst

    data_obj["folders"] = flatten(folders)

    # data.json
    with open(data_json_path, "wb") as out:
        json.dump(data_obj, out, indent = 1)

    print("Writing to %s" % outputfilename)
    with tarfile.open(outputfilename, "w") as tar:
        for root, dirs, files in os.walk(tar_output_dir):
            for name in files:
                relative_dir = os.path.relpath(root, tar_output_dir)
                if relative_dir == ".":
                    tar.add(os.path.join(root, name), arcname=name)
                else:
                    tar.add(
                        os.path.join(
                            root, name), arcname=os.path.join(
                            relative_dir, name))

    #    tarinfo = tarfile.TarInfo.frombuf(json.dumps(data_obj))
    # tarinfo.name = "data.json"
    # file.add(tarinfo)

    # Copy all images to a temporary dir
    # and pack them into a spritesheet

    # for ti in file.getmembers():
    #    if ti.name.startswith("_"):
    #        continue
    #    if ti.isfile() and ti.name.endswith(".png"):
    #        file.extract(ti.name, tmp_dir)

    # Process parts
    #  parts = data_obj["parts"]
    # assert isinstance(parts, dict), "parts not a dictionary"

    

    '''
   
    '''
    
    # First: replace name key with unique id key, and add "name" as a field
    

    #for key, val in parts.iteritems():
    
    '''
    # Replace all the image references in parts
    # with the spritesheet info
    for pname, part in parts.items():
        # print(pname)
        props = {}
        has_remapped_rects = False
        hit_rect_mode = None

        # handle properties
        if "properties" in part.keys():
            # convert string to a json object, to a python object
            # print(mode)
            mode = part["properties"].strip()
            if len(mode) > 0:
                try:
                    props = json.loads(mode)
                except ValueError as ve:
                    print("ERROR: " + str(ve))
                    print("json: \"%s\"" % (mode))
                    print("in part: ", pname, part)
                    raise(ve)
                part["properties"] = props
                if "hit_rect_mode" in props:
                    hit_rect_mode = props["hit_rect_mode"]
                    assert hit_rect_mode in part.keys(), "hit_rect_mode \"" + hit_rect_mode + \
                        "\" not found in " + pname
            else:
                part["properties"] = {}

        for mname, mode in part.items():
            if mname == "properties":
                pass
            else:
                # print(mname)
                for i in range(mode["numFrames"]):
                    frame = mode["frames"][i]
                    image_name = frame["image"]
                    mapped_data = image_map[image_name]
                    frame["image"] = new_spritesheet_name
                    frame["dx"] = mapped_data["spriteSourceSize"]["x"]
                    frame["dy"] = mapped_data["spriteSourceSize"]["y"]
                    frame.update(mapped_data["frame"])

                    # Special case of an empty frame
                    # Texture packer gives it a width,height=1,1
                    # But we'll set it to 0,0 so Moonman engine can ignore
                    # it
                    if frame["dx"] == 0 and frame["dy"] == 0 and frame["w"] == 1 and frame["h"] == 1:
                        # print("Detected empty frame in %s"%pname)
                        frame["w"] = 0
                        frame["h"] = 0

                    if not has_remapped_rects and (
                        (hit_rect_mode is None) or (
                            hit_rect_mode == mname)):
                        has_remapped_rects = True
                        if "properties" in part.keys():
                            props["hit_rect_mode"] = mname
                            part["properties"].update(props)

                        # Transform rectangles - relative to anchor
                        for prop_name, prop in props.items():
                            if prop_name.endswith("_rect"):
                                rect = prop
                                ax, ay = frame["ax"], frame["ay"]
                                rect[0] = rect[0] - ax
                                rect[1] = rect[1] - ay
                                part["properties"][prop_name] = {
                                    "x": rect[0], "y": rect[1], "w": rect[2], "h": rect[3]}

    # Write the modified json file back out (for now)
    data_obj["parts"] = parts
    '''
    '''
    # Process comps
    for comp_name, comp in data_obj["comps"].items():
        if comp_name.startswith("_"):
            continue

        # for each comp we just need to
        # patch up the part refs
        comp["rootIndex"] = comp["root"]
        del comp["root"]
        for part in comp["parts"]:
            if len(part["part"]) == 0:
                part["part"] = ""
            else:
                new_ref = "(sprite/" + part["part"] + ",spritemap)"
                part["part"] = new_ref

        # handle properties
        has_valid_properties = False
        if "properties" in comp:
            # convert string to a json object, to a python object
            # print(mode)
            mode = comp["properties"].strip()
            if len(mode) > 0:
                try:
                    props = json.loads(mode)
                    comp["properties"] = props
                    for prop_name, prop in props.items():
                        if prop_name.endswith("_rect"):
                            rect = prop
                            comp["properties"][prop_name] = {
                                "x": rect[0], "y": rect[1], "w": rect[2], "h": rect[3]}
                    has_valid_properties = True
                except ValueError as e:
                    print(e)
                    print("composition: ", comp_name)
        if not has_valid_properties:
            del comp["properties"]

        filename = os.path.join(package_dir, "sprite", comp_name + ".comp")
        dir = os.path.dirname(filename)
        if not os.path.isdir(dir):
            os.makedirs(dir)
        json.dump(comp, open(filename, "w"), indent=1)
    '''
    print("Success!")

if (len(sys.argv) != 3):
    print("Usage: %s input.tar output.mqs" % sys.argv[0])
else:
    process(sys.argv[1], sys.argv[2])
