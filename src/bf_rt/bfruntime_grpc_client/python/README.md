bfruntime gRPC python client Library (bfruntime-pylib)
====================================

bfruntime-pylib is a library to help interact with BF-RT server over gRPC.

##Structure
The codebase is split into private and public functions and classes. All classes and functions which have leading underscores
are meant for internal usage only. Some classes have leading underscores but contain some functions which are public. Such
classes are called **Partially Internal**.
This indicates that objects of this class are not supposed to be created manually.
Instead, they should be created/retrieved using other methods specified. example -

 ```
# make_data returns back an object of _Data. to_dict() is a public function on _Data
data = table.make_data([gc.DataTuple("port", 1)])
data_dict = data.to_dict()
 ```

###info\_parse.py
It contains helper code to parse bf-rt.json. This file can be used independently and can be used to parse, store
and query bfruntime metadata.

###client.py
It contains a ClientInterface class which can be used to setUp. It also contains Table, Learn and other objects required to
interact with a remote switch. It internally uses info\_parse.py in order to build metadata and query it as and when needed.


