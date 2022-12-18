from __future__ import print_function
import json
import os.path
import shutil
import sys
import tarfile
import tempfile
import zipfile

# This script converts a Version 1 MoonQuest Sprite Project (*.tar) into a Version 2 file (*.mqs)
next_id = 0
def get_next_id():
    global next_id
    next_id += 1
    return next_id


def process(filename, outputfilename):
    temp_dir = tempfile.mkdtemp()

    with tarfile.open(filename, "r") as tar:        
        print("Extracting tar to: %s" % temp_dir)
        def is_within_directory(directory, target):
            
            abs_directory = os.path.abspath(directory)
            abs_target = os.path.abspath(target)
        
            prefix = os.path.commonprefix([abs_directory, abs_target])
            
            return prefix == abs_directory
        
        def safe_extract(tar, path=".", members=None, *, numeric_owner=False):
        
            for member in tar.getmembers():
                member_path = os.path.join(path, member.name)
                if not is_within_directory(path, member_path):
                    raise Exception("Attempted Path Traversal in Tar File")
        
            tar.extractall(path, members, numeric_owner=numeric_owner) 
            
        
        safe_extract(tar, path=temp_dir, members="None")
        data_json_path = os.path.join(temp_dir, "data.json")
        data_obj = json.load(open(data_json_path))

        os.remove(os.path.join(temp_dir, "prefs.json"))

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

    part_list = []
    parts_by_name = {}
    for name, part in data_obj["parts"].iteritems():
        part["id"] = get_next_id()
        names = list(name.split("/"))        
        part_dir = None
        if len(names) > 1:
            part_dir = create_dirs(names[:-1])
        part["name"] = names[-1]
        if part_dir:
            part["parent"] = part_dir["id"]
        # move modes to special sub object
        part["modes"] = []
        for k, v in part.items():
            if type(v) is dict:
                v["name"] = k
                part["modes"].append(v)
                del part[k]
        parts_by_name[name] = part
        part_list.append(part)
    data_obj["parts"] = part_list

    comp_list = []
    for name, comp in data_obj["comps"].iteritems():
        comp["id"] = get_next_id()
        names = list(name.split("/"))
        comp_dir = None
        if len(names) > 1:
            comp_dir = create_dirs(names[:-1])
        comp["name"] = names[-1]
        if comp_dir:
            comp["parent"] = comp_dir["id"]
        for part in comp["parts"]:
            part["id"] = get_next_id()
            part_ref = part["part"] if "part" else ""
            if len(part_ref) > 0:
                assert part_ref in parts_by_name, "%s not in %s"%(str(part_ref), str(parts_by_name.keys()))
                part_ref = parts_by_name[part_ref]["id"]
                part["part"] = part_ref
            else:
                del part["part"]
        comp_list.append(comp)

    data_obj["comps"] = comp_list    

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
    with zipfile.ZipFile(outputfilename, "w", zipfile.ZIP_DEFLATED) as file:
        for root, dirs, files in os.walk(temp_dir):
            for name in files:
                relative_dir = os.path.relpath(root, temp_dir)
                if relative_dir == ".":
                    file.write(os.path.join(root, name), arcname=name)
                else:
                    file.write(
                        os.path.join(
                            root, name), arcname=os.path.join(
                            relative_dir, name))

    print("Success!")

if (len(sys.argv) != 3):
    print("Usage: %s input.tar output.mqs" % sys.argv[0])
else:
    process(sys.argv[1], sys.argv[2])
