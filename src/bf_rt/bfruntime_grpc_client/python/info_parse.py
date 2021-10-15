#
# Copyright(c) 2021 Intel Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import json

logger = logging.getLogger('bfruntime_parse')
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.DEBUG)

def _generate_unique_names(input_name):
    tokens = input_name.split(".")
    name_list = set()
    last_token = ""
    for token in reversed(tokens):
      if not last_token:
        name_list.add(token)
        last_token = token
      else:
        name_list.add(token+"."+last_token)
        last_token = token+"."+last_token
    return name_list


def _create_allname_dict(parent_name_dict, canonical_dict):
    """@brief Same object
        can be referenced by many names depending upon uniqueness.
        This function makes another allname dictionary which will contain many
        names which will point to one canonical name.
    """
    if canonical_dict is None or parent_name_dict is None:
        return
    names_to_remove = set()
    for obj_name, obj_info in list(canonical_dict.items()):
        # Make all possible names for the object and try to insert them
        # in the dictionary. Ignore the full name itself hence remove it
        # from set
        possible_name_set = _generate_unique_names(obj_name)
        for prospective_name in possible_name_set:
            if prospective_name in parent_name_dict:
                names_to_remove.add(prospective_name)
            else:
                parent_name_dict[prospective_name] = obj_name
    for name in names_to_remove:
        parent_name_dict.pop(name, None)


class _TableInfo:
    """ @brief Class _TableInfo (Partially internal). Objects of this class are created during BfRtInfo parsing.
        It contains all metadata information related to a Table. Objects of _TableInfo are
        stored in the table_info_dict in BfRtInfoParser object after the bf-rt.json parsing
        is done. The _TableInfo stores all of its metadata in 3 dictionaries - key, data and
        action. key_dict contains _KeyInfo objects, data dict contains _DataInfo and action
        dict contains _ActionInfo
    """
    def __init__(self, name, table_id, table_type, size, attributes, operations, annotations, has_const_default_action):
        """@brief Create the table object. 
        """
        self.action_dict = {}
        # The allname dictionaries just contain
        # maps of (name -> name) for the objects
        # each object could be referred by more than
        # one name and these dictionaries contain the map
        # from the possible names to the canonical name
        self.action_dict_allname = {}

        self.key_dict = {}
        self.key_dict_allname = {}

        self.data_dict = {}
        self.data_dict_allname = {}

        self.id = table_id
        self.size = size
        self.name = name
        self.table_type = table_type
        self.attributes = attributes
        self.operations = operations
        self.annotations = annotations
        self.has_const_default_action = has_const_default_action

    def id_get(self):
        """@brief Get Table ID
            @return Table ID
        """
        return self.id

    def name_get(self):
        """@brief Get Table Name
            @return Table Name
        """
        return self.name

    def size_get(self):
        """@brief Get Table Size
            @return Table Size
        """
        return self.size

    def type_get(self):
        """@brief Get Table Type
            @return Table Type
        """
        return table_type

    def attributes_supported_get(self):
        """@brief Get Table Attributes Supported
            @return List Table Attributes Supported. List of strings
        """
        return self.attributes

    def operations_supported_get(self):
        """@brief Get Table Attributes Supported
            @return List Table Attributes Supported. List of strings
        """
        return self.operations

    def has_const_default_action_get(self):
        """@brief Get whether table has const default action
            @return bool
        """
        return self.has_const_default_action

    def action_annotations_get(self, action_name):
        """@brief Get list of Action Annotations
            @brief action_name action_name
            @return List list of action _Annotation objects
        """
        return self.action_dict[self.action_dict_allname[action_name]].annotations

    def action_id_get(self, action_name):
        """@brief Get action ID from action name
            @param action_name action_name
            @return Action ID
        """
        return self.action_dict[self.action_dict_allname[action_name]].id

    def action_name_list_get(self):
        """@brief Get list of all action names for this table
            @return List of action names
        """
        return list(self.action_dict.keys())

    def action_name_get(self, action_id):
        """@brief Get action name from action ID
            @param action_id Action ID
            @return Action name
            @exception KeyError on Not finding the action ID
        """
        for action_name_, action_ in list(self.action_dict.items()):
            if action_id == action_.id:
                return action_.name
        raise KeyError("Action ID %n doesn't exist" %(action_id))

    def data_field_name_get(self, field_id, action_name=None):
        """@brief Get Data Field name from field ID.
            @param field_id Field ID
            @param action_name (optional) action name. If an action name is given, then
            this func first searches for the field within the action. If not found, then it
            continues on to the common data fields. This also searches containers
            @return Field name
            @exception KeyError on Not finding the field ID
        """
        def _data_field_name_get_helper(dict_to_search, field_id, ret_list):
            if dict_to_search is None:
                return False
            for field_name_, data_ in list(dict_to_search.items()):
                if data_.id == field_id:
                    ret_list.append(data_.name)
                    return True
            found = False
            for field_name_, field_ in list(dict_to_search.items()):
                found |= _data_field_name_get_helper(field_.container_dict, field_id, ret_list)
            return found

        ret_list = []
        if action_name is not None:
            found = _data_field_name_get_helper(
                    self.action_dict[self.action_dict_allname[action_name]].data_dict,
                    field_id, ret_list)
            if found:
                return ret_list[0]
        found = _data_field_name_get_helper(self.data_dict, field_id, ret_list)
        if found:
            return ret_list[0]
        # Error 404
        if (action_name):
            raise KeyError("Failed to find field %d for action %s in table %s"
                    % (field_id, action_name, self.name))
        else:
            raise KeyError("Failed to find field %d in table %s"
                    % (field_id, self.name))

    def _data_field_get(self, field_name, action_name=None):
        """@brief returns all fields which satisfy field and action name in a
            list. This is to facilitate duplicate fields in a container scenario
        """
        def _data_field_get_helper(dict_to_search, name_dict_to_search, field_name):
            ret_list = []
            if dict_to_search is None:
                return []
            if field_name in name_dict_to_search:
                return [dict_to_search[name_dict_to_search[field_name]]]
            for field_name_, field_ in list(dict_to_search.items()):
                ret_list.extend(_data_field_get_helper(field_.container_dict,
                    field_.container_dict_allname,
                    field_name))
            return ret_list

        field_list = []
        if action_name is not None:
            field_list.extend(_data_field_get_helper(
                self.action_dict[self.action_dict_allname[action_name]].data_dict,
                self.action_dict[self.action_dict_allname[action_name]].data_dict_allname,
                field_name))
        field_list.extend(_data_field_get_helper(self.data_dict, self.data_dict_allname, field_name))
        if len(field_list) > 0:
            return field_list
        #error 404
        if (action_name):
            raise KeyError("Failed to find field %s for action %s in table %s"
                    % (field_name, action_name, self.name))
        else:
            raise KeyError("Failed to find field %s in table %s"
                    % (field_name, self.name))

    def _data_field_metadata_get(self, metadata, field_name, action_name=None):
        fields = self._data_field_get(field_name, action_name)
        # We don't care about duplicates in get since all fields by same name
        # are guaranteed to have same metadata
        field = fields[0]
        if metadata == "id":
            return field.id
        elif metadata ==  "size":
            return field.size
        elif metadata ==  "type":
            return field.type
        elif metadata ==  "repeated":
            return field.repeated
        elif metadata ==  "mandatory":
            return field.mandatory
        elif metadata ==  "read_only":
            return field.read_only
        elif metadata ==  "choices":
            return field.choices
        elif metadata ==  "annotations":
            return field.annotations
        else:
            raise ValueError("Invalid metadata choice")
        return None


    def data_field_id_get(self, field_name, action_name=None):
        """@brief Get Field ID from Field name.
            @param field_name Field name
            @param action_name (optional) action name. If an action name is given, then
            this func first searches for the field within the action. If not found, then it
            continues on to search the common data fields
            @return Field ID
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("id", field_name, action_name)

    def data_field_size_get(self, field_name, action_name=None):
        """@brief Get Field size from Field name. A tuple containing (Bytes, bits) is
            returned. Number of bytes is rounded off to the next largest byte.
            @param field_name Field name
            @param action_name (optional) action name. If an action name is given, then
            this func first searches for the field within the action. If not found, then it
            continues on to search the common data fields
            @return Tuple containing (Bytes, bits)
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("size", field_name, action_name)

    def data_field_type_get(self, field_name, action_name=None):
        """@brief Get Field type from Field name.
            @param field_name Field name
            @param action_name (optional) action name. If an action name is given, then
            this func first searches for the field within the action. If not found, then it
            continues on to search the common data fields
            @return Field Type
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("type", field_name, action_name)

    def data_field_repeated_get(self, field_name, action_name=None):
        """@brief Get whether a field is repeated from Field name.
            @param field_name Field name
            @param action_name (optional) action name. If an action name is given, then
            this func first searches for the field within the action. If not found, then it
            continues on to search the common data fields
            @return Field repeated
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("repeated", field_name, action_name)

    def data_field_mandatory_get(self, field_name, action_name=None):
        """@brief Get whether a field is mandatory from Field name.
            @param field_name Field name
            @param action_name (optional) action name. If an action name is given, then
            this func first searches for the field within the action. If not found, then it
            continues on to search the common data fields
            @return Field mandatory
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("mandatory", field_name, action_name)

    def data_field_read_only_get(self, field_name, action_name=None):
        """@brief Get whether a field is read only from Field name.
            @param field_name Field name
            @param action_name (optional) action name. If an action name is given, then
            this func first searches for the field within the action. If not found, then it
            continues on to search the common data fields
            @return Field read-only
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("read_only", field_name, action_name)

    def data_field_allowed_choices_get(self, field_name, action_name=None):
        """@brief Get Field allowed choices from Field name. This is useful when a field
            type is string
            @param field_name Field name
            @param action_name (optional) action name. If an action name is given, then
            this func first searches for the field within the action. If not found, then it
            continues on to search the common data fields
            @return List of strings which show the allowed choices for this field
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("choices", field_name, action_name)

    def data_field_annotations_get(self, field_name, action_name=None):
        """@brief Get Field annotations
            @param field_name Field name
            @param action_name (optional) action name. If an action name is given, then
            this func first searches for the field within the action. If not found, then it
            continues on to search the common data fields
            @return List of annotations
        """
        return self._data_field_metadata_get("annotations", field_name, action_name)

    def data_field_name_list_get(self, action_name=None):
        """@brief Get List of field names.
            @param action_name (optional) action name. If an action name is given, then
            this func first searches for the field within the action, then it
            continues on to search the common data fields
            @return List of field_names
        """
        ret_field_name_list = []
        if action_name is not None:
            ret_field_name_list=[field_name_ for field_name_, data_ in 
                    list(self.action_dict[self.action_dict_allname[action_name]].data_dict.items())]
        for data_name_, data_ in list(self.data_dict.items()):
            ret_field_name_list.append(data_name_)
        return ret_field_name_list


    def data_field_container_field_name_list_get(self, container_field_name):
        """@brief Get List of field names when a container name is given. Container names if
            repeated in the snapshot metadata, will always contain the same internal metadata, so
            the parent of the container doesn't matter.
            @param container_field_name The container of whose List of field names are required
            @return List of field names
        """
        def dfs_helper(dict_to_find_in, container_field_name, ret_list):
            if dict_to_find_in is None:
                return
            if container_field_name in dict_to_find_in:
                for field_name, field in list(dict_to_find_in[container_field_name].container_dict.items()):
                    ret_list.append(field.name)
                return
            else:
                for field_name, field in list(dict_to_find_in.items()):
                    return dfs_helper(field.container_dict, container_field_name, ret_list)
        ret_list = []
        dfs_helper(self.data_dict, container_field_name, ret_list)
        return ret_list

    def key_field_id_get(self, field_name):
        """@brief Get Key Field ID from Field name.
            @param field_name Field name
            @return Field ID
            @exception KeyError on Not finding the field
        """
        return self.key_dict[self.key_dict_allname[field_name]].id

    def key_field_size_get(self, field_name):
        """@brief Get Field size from Field name. A tuple containing (Bytes, bits) is
            returned. Number of bytes is rounded off to the next largest byte.
            @param field_name Field name
            @return Field size Tuple containing (Bytes, bits)
            @exception KeyError on Not finding the field
        """
        return self.key_dict[self.key_dict_allname[field_name]].size

    def key_field_type_get(self, field_name):
        """@brief Get Key Field type from Field name. (uint64, uint32 etc)
            @param field_name Field name
            @return Field type
            @exception KeyError on Not finding the field
        """
        return self.key_dict[self.key_dict_allname[field_name]].type

    def key_field_match_type_get(self, field_name):
        """@brief Get Key Field match type from Field name. (Exact, Ternary etc)
            @param field_name Field name
            @return Field Match type
            @exception KeyError on Not finding the field
        """
        return self.key_dict[self.key_dict_allname[field_name]].match_type

    def key_field_repeated_get(self, field_name):
        """@brief Get Key whether field is repeated from Field name.
            @param field_name Field name
            @return Is the field repeated
            @exception KeyError on Not finding the field
        """
        return self.key_dict[self.key_dict_allname[field_name]].repeated
    
    def key_field_mandatory_get(self, field_name):
        """@brief Get Key whether field is mandatory from Field name.
            @param field_name Field name
            @return Is the field mandatory
            @exception KeyError on Not finding the field
        """
        return self.key_dict[self.key_dict_allname[field_name]].mandatory
    

    def key_field_name_get(self, field_id):
        """@brief Get Key Field Name from ID.
            @param field_id Field ID
            @return Field Name
            @exception KeyError on Not finding the field
        """
        for field_name_, key_ in list(self.key_dict.items()):
            if key_.id == field_id:
                return key_.name
        raise KeyError("Failed to find %d as key in table %s" % (field_id, self.name))

    def key_field_annotations_get(self, field_name):
        """@brief Get Field annotations
            @param field_name Field name
            @return List of annotations
        """
        return self.key_dict[self.key_dict_allname[field_name]].annotations

    def key_field_name_list_get(self):
        """@brief Get List of Key Field names.
            @return List of Key Field names
        """
        return list(self.key_dict.keys())

    def data_field_annotation_add(self, field_name, action_name, custom_annotation):
        """@brief Add a custom annotation like ipv4, ipv6, mac, bytes etc. The description of this
            functions refers to client.py but it doesn't create any dependency with it.
            If a custom annotation is set, then certain extra types become activated on a DataTuple.
            @param field_name Field name
            @param action_name Action Name
            @param custom_annotation can be the following ->
            ipv4: make_data can accept string of format "10.12.14.16". A to_dict on data will return string
            mac: make_data can accept string of format "4f:3d:2c:1a:00:ff". A to_dict on data will return string
            ipv6: make_data can accept string of format "0000:0000:0000:0000:0000:0000:0000:0000".
                A to_dict on data will return string.
            bytes: make_data can always accept bytearrays for valid fields. But if this annotaion is set, then a
            to_dict on data will return a bytearray instead of an int(default)
        """
        fields = self._data_field_get(field_name, action_name)
        for field in fields:
            field.annotations.append(_Annotation("$client_annotation", custom_annotation))

    def key_field_annotation_add(self, field_name, custom_annotation):
        """@brief Add a custom annotation like ipv4, ipv6, mac, bytes etc. The description of this
            functions refers to client.py but it doesn't create any dependency with it.
            If a custom annotation is set, then certain extra types become activated on a KeyTuple.
            @param field_name Field name
            @param custom_annotation can be the following ->
            ipv4: make_key can accept string of format "10.12.14.16". A to_dict on key will return string
            mac: make_key can accept string of format "4f:3d:2c:1a:00:ff". A to_dict on key will return string
            ipv6: make_key can accept string of format "0000:0000:0000:0000:0000:0000:0000:0000".
                A to_dict on key will return string.
            bytes: make_key can always accept bytearrays for valid fields. But if this annotaion is set, then a
            to_dict on key will return a bytearray instead of an int(default)
        """
        self.key_dict[self.key_dict_allname[field_name]].annotations.append(
                _Annotation("$client_annotation",custom_annotation))

class _LearnInfo:
    """ @brief Class _LearnInfo (Partial Internal). Objects of this class are created during BfRtInfo parsing.
        It contains all metadata information related to a Learn object. Objects of _LearnInfo are
        stored in the learn_info_dict in BfRtInfoParser object after the bf-rt.json parsing
        is done
    """
    def __init__(self, learn_name, learn_id, annotations):
        """@brief create the _LearnInfo object
           @param learn_name Name of the learn object
           @param learn_id ID of the learn object
        """
        self.data_dict = {}
        self.data_dict_allname = {}
        self.id = learn_id
        self.name = learn_name
        self.annotations = annotations

    def name_get(self):
        """@brief Get the name of the Learn Object
           @return Name of the learn object
        """
        return self.name

    def id_get(self):
        """@brief Get the ID of the object
           @return ID of the object
        """
        return self.id

    def data_field_name_get(self, field_id, *unused):
        """@brief Get Data Field name from field ID.
            @param field_id Field ID
            @return Field name
            @exception KeyError on Not finding the field ID
        """
        for field_name_, data_ in list(self.data_dict.items()):
            if data_.id == field_id:
                return data_.name
        raise KeyError("Field %d not found in learn obj %s"%(field_id, self.name_get()))

    def _data_field_metadata_get(self, metadata, field_name):
        field = self.data_dict[self.data_dict_allname[field_name]]
        if metadata == "id":
            return field.id
        elif metadata ==  "size":
            return field.size
        elif metadata ==  "type":
            return field.type
        elif metadata ==  "repeated":
            return field.repeated
        elif metadata ==  "mandatory":
            return field.mandatory
        elif metadata ==  "annotations":
            return field.annotations
        else:
            raise ValueError("Invalid metadata choice")
        return None

    def data_field_id_get(self, field_name, *unused):
        """@brief Get Field ID from Field name.
            @param field_name Field name
            @return Field ID
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("id", field_name)

    def data_field_size_get(self, field_name, *unused):
        """@brief Get Field size from Field name. A tuple containing (Bytes, bits) is
            returned. Number of bytes is rounded off to the next largest byte.
            @param field_name Field name
            @return Field size (Bytes, bits)
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("size", field_name)

    def data_field_type_get(self, field_name, *unused):
        """@brief Get Field type from Field name.
            @param field_name Field name
            @return Field type
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("type", field_name)

    def data_field_repeated_get(self, field_name, *unused):
        """@brief Get whether a field is repeated from Field name.
            @param field_name Field name
            @return Field repeated
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("repeated", field_name)

    def data_field_mandatory_get(self, field_name, *unused):
        """@brief Get whether a field is mandatory from Field name. This function is
            not very useful for learn obj since there isn't a notion of a field being
            mandatory but it is required since client application uses it.
            @param field_name Field name
            @return Field mandatory
            @exception KeyError on Not finding the field
        """
        return self._data_field_metadata_get("mandatory", field_name)

    def data_field_annotations_get(self, field_name, *unused):
        """@brief Get Field annotations
            @param field_name Field name
            @return List of annotations
        """
        return self._data_field_metadata_get("annotations", field_name)

    def data_field_name_list_get(self, *unused):
        """@brief Get List of field names.
            @param field_name Field name
            @return List of field_names
        """
        return [data_name_ for data_name_, data_ in list(self.data_dict.items())]

    def data_field_annotation_add(self, field_name, custom_annotation):
        """@brief Add a custom annotation like ipv4, ipv6, mac, bytes etc. The description of this
            functions refers to client.py but it doesn't create any dependency with it.
            If a custom annotation is set, then certain extra types become activated on a DataField.
            @param field_name Field name
            @param custom_annotation can be the following ->
            ipv4: make_data can accept string of format "10.12.14.16". A to_dict on data will return string
            mac: make_data can accept string of format "4f:3d:2c:1a:00:ff". A to_dict on data will return string
            ipv6: make_data can accept string of format "0000:0000:0000:0000:0000:0000:0000:0000".
                A to_dict on data will return string.
            bytes: make_data can always accept bytearrays for valid fields. But if this annotaion is set, then a
            to_dict on data will return a bytearray instead of an int(default)
        """
        self.data_dict[self.data_dict_allname[field_name]].annotations.append(
                _Annotation("$client_annotation", custom_annotation))

class _Annotation:
    """@brief Internal. Store annotations. If compared against strings then it is compared as "name.value"
    """
    def __init__(self, name, value):
        self.name = name
        self.value = value

    # Compares itself to a string object
    def __eq__(self, other):
        return (self.name + "." + self.value) == (other)

class _DataInfo:
    """@brief Internal. Store metadata of a Data Field.
    """
    def __init__(self, name, field_id, field_type, field_size,\
            repeated, mandatory, read_only, annotations, container_dict=None):
        self.name = name
        self.id = field_id
        self.type = field_type
        self.size = field_size
        self.choices = field_type
        self.repeated = repeated
        self.mandatory = mandatory
        self.read_only = read_only
        self.id = field_id
        self.annotations = annotations
        self.container_dict = container_dict
        self.container_dict_allname = dict()
        _create_allname_dict(self.container_dict_allname, self.container_dict)


class _KeyInfo:
    """@brief Internal. Store metadata of a Key Field.
    """
    def __init__(self, name, field_id, field_type, field_size, match_type, repeated, mandatory,
            annotations):
        self.name = name
        self.id = field_id
        self.type = field_type
        self.size = field_size
        self.match_type = match_type
        self.repeated = repeated
        self.mandatory = mandatory
        self.annotations = annotations
        # ATCAM match_type is treated just like Exact match_type.
        if self.match_type == "ATCAM":
            self.match_type = "Exact"

class _ActionInfo:
    """@brief Internal. Store metadata of an action
    """
    def __init__(self, action_name, action_id, annotations):
        self.data_dict = {}
        self.data_dict_allname = {}
        self.name = action_name
        self.id = action_id
        self.annotations = annotations

class BfRtInfoParser:
    """@brief Parse bf-rt.json. This object keeps metadata for tables and learn objects as _TableInfo
        and _LearnInfo objects.
    """
    def __init__(self, p4_json_data, non_p4_json_data):
        self.table_info_dict = {}
        self.learn_info_dict = {}

        # parse tables
        bfrtinfo_json = json.loads(p4_json_data.decode('utf-8'))
        non_p4_json = json.loads(non_p4_json_data.decode('utf-8'))
        for table_json in bfrtinfo_json["tables"]:
            self.table_info_dict[table_json["name"]] = BfRtInfoParser._parse_table(table_json)
        for table_json in non_p4_json["tables"]:
            self.table_info_dict[table_json["name"]] = BfRtInfoParser._parse_table(table_json)
        # parse learn
        if "learn_filters" in bfrtinfo_json:
            for learn_json in bfrtinfo_json["learn_filters"]:
                self.learn_info_dict[learn_json["name"]] = BfRtInfoParser._parse_learn(learn_json)

    def __str__(self):
        msg = "TABLES->\n"
        for table_name, table in list(self.table_info_dict.items()):
            msg += table_name + ":" + str(table.id) + "\n"
            msg += "\tKEYS->\n"
            for key_name, key in list(table.key_dict.items()):
                msg += "\t\t" + key_name + ":" + str(key.id) + "\n"
            msg += "\tACTIONS->\n"
            for action_name, action in list(table.action_dict.items()):
                msg += "\t\t" + action_name + ":" + str(action.id) + "\n"
                msg += BfRtInfoParser._print_data(action.data_dict, 2, "DATA")
            msg += BfRtInfoParser._print_data(table.data_dict, 1, "DATA")
        return msg

    @staticmethod
    def _print_data(data_dict, base_tab_spaces, base_name):
        msg = ""
        msg += ("\t"*base_tab_spaces) + base_name +"->\n"
        for data_name, data in list(data_dict.items()):
            msg += ("\t"*(base_tab_spaces+1)) + data_name + ":" + str(data.id) + "\n"
            if (data.container_dict is not None):
                msg += BfRtInfoParser._print_data(data.container_dict,
                        base_tab_spaces+ 1, data_name)
        return msg

    def table_info_dict_get(self):
        """@brief Returns the table info dictionary
        """
        return self.table_info_dict

    def learn_info_dict_get(self):
        """@brief Returns the Learn Object info dictionary
        """
        return self.learn_info_dict

    @staticmethod
    def _parse_table(table_json):
        annotations = []
        if "annotations" in table_json:
            annotations = BfRtInfoParser._parse_annotations(table_json["annotations"])
        has_const_default_action = False
        if "has_const_default_action" in table_json:
            has_const_default_action = table_json["has_const_default_action"]
        table_info = _TableInfo(table_json["name"],
                table_json["id"],
                table_json["table_type"],
                table_json["size"],
                table_json["attributes"],
                table_json["supported_operations"],
                annotations,
                has_const_default_action)
        for key_json in table_json["key"]:
            table_info.key_dict[key_json["name"]] = BfRtInfoParser._parse_key(key_json)
        _create_allname_dict(table_info.key_dict_allname, table_info.key_dict)
        if ("action_specs" in table_json):
            for action_json in table_json["action_specs"]:
                table_info.action_dict[action_json["name"]] = BfRtInfoParser._parse_action(action_json)
            _create_allname_dict(table_info.action_dict_allname, table_info.action_dict)

        for data_json in table_json["data"]:
            BfRtInfoParser._parse_data_helper(table_info.data_dict, data_json)
        _create_allname_dict(table_info.data_dict_allname, table_info.data_dict)
        return table_info

    @staticmethod
    def _parse_key(key_json):
        field_type, field_size = BfRtInfoParser._parse_field_type(key_json["type"])
        mandatory = key_json["mandatory"] if "mandatory" in key_json else False
        repeated = key_json["repeated"] if "repeated" in key_json else False
        annotations = []
        if "annotations" in key_json:
            annotations = BfRtInfoParser._parse_annotations(key_json["annotations"])

        key_info = _KeyInfo(key_json["name"], key_json["id"],
                field_type, field_size, key_json["match_type"],
                repeated, mandatory,
                annotations)
        return key_info

    @staticmethod
    def _parse_action(action_json):
        annotations = []
        if "annotations" in action_json:
            annotations = BfRtInfoParser._parse_annotations(action_json["annotations"])
        action_info = _ActionInfo(action_json["name"],
                            action_json["id"],
                            annotations)
        for data_json in action_json["data"]:
            BfRtInfoParser._parse_data_helper(action_info.data_dict, data_json)
        _create_allname_dict(action_info.data_dict_allname, action_info.data_dict)
        return action_info

    @staticmethod
    def _parse_data_helper(input_dict, data_json):
        if "oneof" in data_json:
            input_dict[data_json["oneof"][0]["name"]] = BfRtInfoParser._parse_data(
                    data_json["oneof"][0],
                    data_json["mandatory"],
                    data_json["read_only"])
            input_dict[data_json["oneof"][1]["name"]] = BfRtInfoParser._parse_data(
                    data_json["oneof"][1],
                    data_json["mandatory"],
                    data_json["read_only"])
        elif "singleton" in data_json:
            container_dict = None
            if "container" in data_json["singleton"]:
                container_dict = {}
                # Containers contains a set of fields that can be repeated.
                # A container can contain another container.
                # Add a dictionary in 
                for container_data_json in data_json["singleton"]["container"]:
                    BfRtInfoParser._parse_data_helper(container_dict, container_data_json)
            input_dict[data_json["singleton"]["name"]] = BfRtInfoParser._parse_data(
                    data_json["singleton"],
                    data_json["mandatory"],
                    data_json["read_only"],
                    container_dict)
        else:
            input_dict[data_json["name"]] = BfRtInfoParser._parse_data(data_json)

    @staticmethod
    def _parse_data(data_json, mandatory=None, read_only=None, container_dict=None):
        if mandatory is None and "mandatory" in data_json:
            mandatory = data_json["mandatory"]
        if read_only is None and "read_only" in data_json:
            read_only = data_json["read_only"]
        field_type, field_size = None, None
        if container_dict is None:
            field_type, field_size = BfRtInfoParser._parse_field_type(data_json["type"])
        else:
            field_type, field_size = "container", (None, None)
        annotations = []
        if "annotations" in data_json:
            annotations = BfRtInfoParser._parse_annotations(data_json["annotations"])

        data_info = _DataInfo(data_json["name"], data_json["id"],
                field_type, field_size, data_json["repeated"],
                mandatory, read_only, annotations, container_dict)
        return data_info
    
    @staticmethod
    def _parse_annotations(annotations_json):
        return [_Annotation(a["name"], a["value"] if "value" in a else "") for a in annotations_json]

    @staticmethod
    def _parse_learn(learn_json):
        learn_info = _LearnInfo(learn_json["name"],
                        learn_json["id"],
                        BfRtInfoParser._parse_annotations(learn_json["annotations"]))
        for data_json in learn_json["fields"]:
            BfRtInfoParser._parse_data_helper(learn_info.data_dict, data_json)
        _create_allname_dict(learn_info.data_dict_allname, learn_info.data_dict)
        return learn_info

    @staticmethod
    def _parse_field_type(field_type):
        if field_type is None:
            return None, (None, None)

        ftype = field_type["type"]
        fsize = (None, None)

        if ftype == "bytes":
            fbits = int(field_type["width"])
            fsize = ((fbits+7)//8 , fbits)
        elif ftype == "uint64":
            fsize = (8, 8*8)
        elif ftype == "uint32":
            fsize = (4, 4*8)
        elif ftype == "uint16":
            fsize = (2, 2*8)
        elif ftype == "uint8":
            fsize = (1, 1*8)
        elif ftype == "bool":
            fsize = (None, None)
        elif ftype == "float":
            fsize = (None, None)
        elif ftype == "string":
            fsize = (None, None)
        elif ftype == "enum":
            # enum is treated as a string with allowed choices
            fsize = (None, None)
            ftype = "string"
        else:
            raise KeyError("Unknown field type \"%s\" found", ftype)
        return ftype, fsize
