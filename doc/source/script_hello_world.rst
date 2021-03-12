Hello world
===========

.. default-domain:: lua
   
A flagging strategy script is a Lua script file that defines
one or more execution functions, normally named :meth:`execute`. It may 
additionally define an :meth:`options` function to set options.

For a simple strategy, a flagging script can consist of a single ``execute``
function:

.. code-block:: lua

   function execute (data)
     print("Hello world: " ..
           data:get_antenna1_name() ..
           " x " ..
           data:get_antenna2_name() )
   end

The Lua ``print``
statement writes something to the console. Two dots are used in Lua to
concatenate strings. Let's assume that the above
script is saved as ``helloworld.lua``. This strategy can now be run on a
dataset as follows:

.. code-block:: bash

   aoflagger -strategy helloworld.lua observation.ms
   
When running this strategy on a WSRT dataset, the output looks like this:

.. code-block:: none

    $ aoflagger -strategy helloworld.lua observation.ms
    AOFlagger 3.0-alpha (2020-03-06) command line application
    Author: André Offringa (offringa@gmail.com)

    Starting strategy on 2020-Jun-21 22:26:56.154492
    Hello world: RT0 x RT2
    Hello world: RT0 x RT1
    Hello world: RT0 x RT3
    Hello world: RT0 x RT5
    Hello world: RT0 x RT4
    Hello world: RT0 x RT7
    Hello world: RT0 x RT6
    Hello world: RT0 x RT9
    Hello world: RT0 x RT8
    Hello world: RT0 x RTB
    Hello world: RT0 x RTA
    Hello world: RT0 x RTD
    Hello world: RT0 x RTC
    Hello world: RT1 x RT3
    Hello world: RT1 x RT2
    ...

As can be seen from the output,
the ``execute()`` function is called for every pair of antennas (antennas are
named RT0/1/2/... for the WSRT). For every
antenna pair, the dynamic spectrum data is loaded and passed to ``execute()``.
Additionally, the existing flags and some metadata are loaded. These
data are all accessible through the ``data`` object (see class :class:`Data`).
Because this strategy does not modify the ``data`` object, the existing flags
are written back to the data set, so nothing is changed.

When the data set is from an interferometric telescope,
such as in this example, the default option is to only flag the
cross-correlations. Because the function is called from multiple
independent threads, the order in which the baselines are processed can
differ.

Defining options
^^^^^^^^^^^^^^^^

If a function named ``options`` exist, ``aoflagger`` will call this function
before opening the dataset, and expects it to return a table with options.
Here is the same hello world strategy that processes both
auto-correlations and cross-correlations:

.. code-block:: lua

   function execute (data)
     print("Hello world for " ..
           data:get_antenna1_name() ..
           " x " ..
           data:get_antenna2_name() )
   end
    
   function options ()
     main_options = { }
     main_options["baselines"] = "all"
     return { ["main"] = main_options }
   end

If ``aoflagger`` is again used to run this strategy, the auto-correlations
are indeed also shown. This is achieved by setting the ``"baselines"`` option
to ``"all"``. See the :doc:`list of options <strategy_options>` for other
options.
   
The ``options()`` function returns a table of tables. Elements of the first
table can enumerate one or more runs, whereas the second table define the
options. With this system, it is possible to define multiple execute functions.
These will process
the data one after each other. Be aware that the dataset is first completely
processed by the first function, then completely processed by the second
function, etc.

.. note ::

   Multiple execute functions cause the data to be read multiple times.

Multiple passes can be useful to e.g. first flag all baselines separately, and
then perform flagging based on the integrated statistics.
   
The following script sets up the options to perform two runs
through the data:
  
.. code-block:: lua

   function execute_per_baseline (data)
     -- Modify the 'data' object here
   end
    
   function execute_integrated (data)
     -- Modify the 'data' object here
   end
    
   function options ()
     opts1 = {
       ["baselines"] = "all"
       ["execute_function"] = "execute_per_baseline"
     }
     
     opts2 = {
       ["baselines"] = "cross"
       ["baseline_integration"] = "average"
       ["execute_function"] = "execute_integrated"
     }
     
     return {
       ["per baseline"] = opts1,
       ["baseline-integrated"] = opts2
     }
   end
 
``rfigui``, ``aoflagger`` and the API make use of the same Lua structure, i.e., a
script that runs in ``rfigui`` can also be run by ``aoflagger`` and by the
:doc:`C++ <cpp_interface>` and :doc:`Python <python_interface>` interfaces.
However, not all options are relevant for the ``rfigui`` or interfaces. 

