Lua strategies
==============

AOFlagger flagging strategies are written in the `Lua language <https://www.lua.org/>`_.
Lua is quite a simple language, and if you know Python, Lua will feel quite familiar.
Most of its syntax should be evident when inspecting some of the example
strategies. In Lua, comments start with two dashes "``--``", ``if`` and ``for`` blocks
end with an ``end`` statement, and class methods can be called with either

.. code-block:: lua

    object.method(object, parameter1, parameter2)
    
or

.. code-block:: lua

    object:method(parameter1, parameter2)

.. toctree::
   :maxdepth: 2
   :caption: The following sections describe how to use Lua to design strategies:
   
   script_hello_world
   script_structure
   script_example
   Option list <strategy_options>
   functions
   class_data
   from_rfis_to_lua
