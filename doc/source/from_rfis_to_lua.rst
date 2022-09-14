Upgrading .rfis files
=====================

If you have existing ``.rfis`` files but would like to use AOFlagger 3.0, you will have to upgrade those old ``.rfis`` files to Lua files. While Lua strategies can do everything that the old ``.rfis`` files could do, there is no automated method to convert them, because not every function in the old ``.rfis`` maps one to one to a Lua function. This unfortunately means you might have to do some manual work to make a Lua file. The good news is that Lua strategies are much more powerful.

The easiest way to create a Lua file is to start from an existing strategy, such as the default Lua strategy. If you started the old ``.rfis`` strategy from a default strategy as well, and you remember the changes necessary for the ``.rfis`` strategy, it should in most cases be simple to modify the default Lua strategy. If the modifications are unknown, I recommend to use a tool such as ``diff`` or ``meld`` to visualize the changes. Because the ``.rfis`` files are XML files, this should in most cases produce a human-parsable list of differences.

As a simple example, let's say that a ``diff -Naur default.rfis modified.rfis``, i.e. the difference between aoflagger's default strategy and your own strategy, shows this:

.. code-block:: diff

  --- default.rfis   2021-09-17 10:12:44.116199932 +0200
  +++ modified.rfis   2021-09-17 10:11:10.094925901 +0200
  @@ -31,8 +31,8 @@
   <sensitivity-start>4</sensitivity-start>
   <children>
     <action type="SumThresholdAction">
  -    <time-direction-sensitivity>1</time-direction-sensitivity>
  -    <frequency-direction-sensitivity>1</frequency-direction-sensitivity>
  +    <time-direction-sensitivity>1.5</time-direction-sensitivity>
  +    <frequency-direction-sensitivity>1.5</frequency-direction-sensitivity>
      <time-direction-flagging>1</time-direction-flagging>
      <frequency-direction-flagging>1</frequency-direction-flagging>
      <exclude-original-flags>0</exclude-original-flags>
  @@ -79,8 +79,8 @@
      </children>
    </action>
    <action type="SumThresholdAction">
  -    <time-direction-sensitivity>1</time-direction-sensitivity>
  -    <frequency-direction-sensitivity>1</frequency-direction-sensitivity>
  +    <time-direction-sensitivity>1.5</time-direction-sensitivity>
  +    <frequency-direction-sensitivity>1.5</frequency-direction-sensitivity>
      <time-direction-flagging>1</time-direction-flagging>
      <frequency-direction-flagging>1</frequency-direction-flagging>
      <exclude-original-flags>0</exclude-original-flags>

This shows that the sensitivity (both time and frequency direction) of the SumThreshold actions were changed from 1 to 1.5. In the old ``.rfis`` strateges, the sensitivity values are specified every time an action is executed. Because the SumThresholdAction occurs two times, there are two different places in the ``.rfis`` file. This is not the case for Lua files: in Lua, one can define variables, and the default strategy makes use of this. It lists the most commonly used parameters near the start of the Lua file, and changing the sensitivity of the SumThreshold step can be done simply by changing the base_threshold variable from its default:

.. code-block:: lua
  
  local base_threshold = 1.0  -- lower means more sensitive detection

to:

.. code-block:: lua
  
  local base_threshold = 1.5  -- lower means more sensitive detection
