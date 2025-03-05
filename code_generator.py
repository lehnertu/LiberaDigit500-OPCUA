import xml.etree.ElementTree as ET
from copy import deepcopy

class Folder(dict):
    def __init__(self, name, parent_node_id):
        super().__init__(name=name, parent_node_id=parent_node_id)  
        self.update({'node_id': f'{name}Folder'})
    def generate_main_code(self):
        code = f'''    object_attr = UA_ObjectAttributes_default;\n'''
        code += f'''    object_attr.description = UA_LOCALIZEDTEXT("en_US","{self['description']}");\n'''
        code += f'''    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","{self['name']}");\n'''
        code += f'''    static UA_NodeId {self['node_id']};\n'''
        code += f'''    UA_Server_addObjectNode(\n'''
        code += f'''            server,\n'''
        code += f'''            UA_NODEID_STRING(1, "{self['name']}"),\n'''
        code += f'''            {self['parent_node_id']},\n'''
        code += f'''            UA_NS0ID(ORGANIZES),\n'''
        code += f'''            UA_QUALIFIEDNAME(1, "{self['name']}"),\n'''
        code += f'''            UA_NS0ID(FOLDERTYPE),\n'''
        code += f'''            object_attr, \n'''
        code += f'''            NULL,\n'''
        code += f'''            &{self['node_id']});\n'''
        return code

class Node(dict):
    def __init__(self, name, parent_node_id):
        super().__init__(name=name, parent_node_id=parent_node_id)  
    def generate_mci_header(self):
        code = f'''bool mci_get_{self['function']}({self['mci_type']} *val);\n'''
        code += f'''bool mci_set_{self['function']}({self['mci_type']} val);\n'''
        return code
    def generate_mci_code(self):
        code = f'''bool mci_get_{self['function']}({self['mci_type']} *val)\n'''
        code += '{\n'
        code += f'''    {self['mci_type']} mci_val;\n'''
        code += f'''    bool success = node_{self['function']}.GetValue(mci_val);\n'''
        code += f'''    *val = mci_val;\n'''
        code += f'''    return success;\n'''
        code += '}\n'
        code += f'''bool mci_set_{self['function']}({self['mci_type']} val)\n'''
        code += '{\n'
        code += f'''    bool success = node_{self['function']}.SetValue(val);\n'''
        code += f'''    return success;\n'''
        code += '}\n'
        return code
    def generate_mci_nodes(self):
        code = f'''mci::Node node_{self['function']};\n'''
        return code
    def generate_mci_init(self):
        code = f'''    node_{self['function']} = node_root.GetNode(mci::Tokenize("{self['token_string']}"));\n'''
        code += f'''    if (node_{self['function']}.IsValid())\n'''
        code += '    {\n'
        code += f'''        int valtype = node_{self['function']}.GetValueType();\n'''
        code += f'''        printf("{self['token_string']} type %d\\n", valtype);\n'''
        code += '    } else {\n'
        code += f'''        mci_error = 2;\n'''
        code += f'''        printf("MCI node error : {self['token_string']}\\n");\n'''
        code += '    };\n'
        return code
    def generate_opcua_header(self):
        code = f'''UA_StatusCode get_{self['function']}(\n'''
        code += f'''    UA_Server *server,\n'''
        code += f'''    const UA_NodeId *sessionId, void *sessionContext,\n'''
        code += f'''    const UA_NodeId *nodeId, void *nodeContext,\n'''
        code += f'''    UA_Boolean sourceTimeStamp,\n'''
        code += f'''    const UA_NumericRange *range,\n'''
        code += f'''    UA_DataValue *dataValue);\n'''
        code += f'''UA_StatusCode set_{self['function']}(\n'''
        code += f'''    UA_Server *server,\n'''
        code += f'''    const UA_NodeId *sessionId, void *sessionContext,\n'''
        code += f'''    const UA_NodeId *nodeId, void *nodeContext,\n'''
        code += f'''    const UA_NumericRange *range,\n'''
        code += f'''    const UA_DataValue *data);\n'''
        return code
    def generate_opcua_code(self):
        code = f'''UA_StatusCode get_{self['function']}(\n'''
        code += f'''    UA_Server *server,\n'''
        code += f'''    const UA_NodeId *sessionId, void *sessionContext,\n'''
        code += f'''    const UA_NodeId *nodeId, void *nodeContext,\n'''
        code += f'''    UA_Boolean sourceTimeStamp,\n'''
        code += f'''    const UA_NumericRange *range,\n'''
        code += f'''    UA_DataValue *dataValue)\n'''
        code += '''{\n'''
        code += f'''    {self['mci_type']} val;\n'''
        code += f'''    if(mci_get_{self['function']}(&val))\n'''
        code += '''    {\n'''
        code += f'''        UA_Variant_setScalarCopy(&dataValue->value, ({self['ua_type']}*)(&val), &UA_TYPES[{self['ua_type_desc']}]);\n'''
        code += f'''        dataValue->hasValue = true;\n'''
        code += f'''        return UA_STATUSCODE_GOOD;\n'''
        code += '''    }\n'''
        code += f'''    else\n'''
        code += '''    {\n'''
        code += f'''        printf("MCI value error : get : {self['token_string']}\\n");\n'''
        code += f'''        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;\n'''
        code += '''    }\n'''
        code += '''}\n'''
        code += f'''UA_StatusCode set_{self['function']}(\n'''
        code += f'''    UA_Server *server,\n'''
        code += f'''    const UA_NodeId *sessionId, void *sessionContext,\n'''
        code += f'''    const UA_NodeId *nodeId, void *nodeContext,\n'''
        code += f'''    const UA_NumericRange *range,\n'''
        code += f'''    const UA_DataValue *data)\n'''
        code += '''{\n'''
        code += f'''    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[{self['ua_type_desc']}]) && (data->value.data != 0))\n'''
        code += '''    {\n'''
        code += f'''        {self['mci_type']} val = *({self['mci_type']}*)data->value.data;\n'''
        code += f'''        if(mci_set_{self['function']}(val))\n'''
        code += f'''            return UA_STATUSCODE_GOOD;\n'''
        code += f'''        else\n'''
        code += '''        {\n'''
        code += f'''            printf("MCI value error : set : {self['token_string']}\\n");\n'''
        code += f'''            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;\n'''
        code += '''        }\n'''
        code += '''    }\n'''
        code += f'''    else\n'''
        code += '''    {\n'''
        code += f'''		printf("data error : set_{self['function']}\\n");\n'''
        code += f'''        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;\n'''
        code += '''    }\n'''
        code += '''}\n'''
        return code
    def generate_main_code(self):
        code = f'''    attr = UA_VariableAttributes_default;\n'''
        code += f'''    attr.description = UA_LOCALIZEDTEXT("en_US","{self['description']}");\n'''
        code += f'''    attr.displayName = UA_LOCALIZEDTEXT("en_US","{self['name']}");\n'''
        code += f'''	attr.valueRank = UA_VALUERANK_SCALAR;\n'''
        code += f'''    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;\n'''
        code += f'''    UA_DataSource {self['function']}_DataSource = (UA_DataSource)\n'''
        code += '''        {\n'''
        code += f'''            .read = get_{self['function']},\n'''
        code += f'''            .write = set_{self['function']}\n'''
        code += '''        };\n'''
        code += f'''    UA_Server_addDataSourceVariableNode(\n'''
        code += f'''            server,\n'''
        code += f'''            UA_NODEID_STRING(1, "{self['name']}"),\n'''
        code += f'''            {self['parent_node_id']},\n'''
        code += f'''            UA_NS0ID(ORGANIZES),\n'''
        code += f'''            UA_QUALIFIEDNAME(1, "{self['name']}"),\n'''
        code += f'''            UA_NS0ID(BASEDATAVARIABLETYPE),\n'''
        code += f'''            attr,\n'''
        code += f'''            {self['function']}_DataSource,\n'''
        code += f'''            NULL,\n'''
        code += f'''            NULL);\n'''
        return code

# Load the XML file
tree = ET.parse("variables.xml")
root = tree.getroot()

# -----------------------------------------------
# create a complete listing of the tree structure
# -----------------------------------------------

List_of_Folders = []
List_of_Nodes = []

def traverse_tree(xml_node, parent_folder):
    # handle all <folder> children
    for f in xml_node.findall("folder"):
        new_f = Folder(name=f.get("name"), parent_node_id=parent_folder['node_id'])
        new_f.update({'description':f.get("description")})
        List_of_Folders.append(deepcopy(new_f))
        print('new folder:', new_f)
        # recursive call
        traverse_tree(f, new_f)
    # handle all <node> children
    for f in xml_node.findall("node"):
        new_n = Node(name=f.get("name"), parent_node_id=parent_folder['node_id'])
        # copy all attributes from the XML node into the Node dict
        new_n.update(dict(f.attrib))
        List_of_Nodes.append(deepcopy(new_n))
        print('new node:', new_n)

root_folder=Folder(name="OBJECTSFOLDER", parent_node_id="None")
root_folder.update({'node_id':"UA_NS0ID(OBJECTSFOLDER)"})

print('root folder:', root_folder)
traverse_tree(root, root_folder)
print()
print('done with the tree.')
print()

for n in List_of_Nodes:
    print('Node: ', n)
# -------------
# generate code
# -------------

fd = open('OpcUaServer.c.inc', 'w')
for f in List_of_Folders:
    fd.write(f.generate_main_code())
for n in List_of_Nodes:
    fd.write(n.generate_main_code())
fd.close()

fd = open('libera_mci.h.inc', 'w')
for n in List_of_Nodes:
    fd.write(n.generate_mci_header())
fd.close()

fd = open('libera_mci.cpp.inc', 'w')
for n in List_of_Nodes:
    if not "no_mci_code" in n.keys():
        fd.write(n.generate_mci_code())
fd.close()

fd = open('libera_mci.cpp.nodes.inc', 'w')
for n in List_of_Nodes:
    fd.write(n.generate_mci_nodes())
fd.close()

fd = open('libera_mci.cpp.init.inc', 'w')
for n in List_of_Nodes:
    fd.write(n.generate_mci_init())
fd.close()

fd = open('libera_opcua.h.inc', 'w')
for n in List_of_Nodes:
    fd.write(n.generate_opcua_header())
fd.close()

fd = open('libera_opcua.c.inc', 'w')
for n in List_of_Nodes:
    fd.write(n.generate_opcua_code())
fd.close()

