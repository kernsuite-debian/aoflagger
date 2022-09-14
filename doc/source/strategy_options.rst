Strategy option list
====================

.. default-domain:: lua
 
The following table lists all the options that can be set when implementing the :meth:`options` function.

============================ =======     =========== 
option                       type        description
============================ =======     ===========
bands                        table       List of integer (zero-indexed) band ids to process.
baselines                    string      ``"auto"``, ``"cross"`` or ``"all"`` for selecting auto/cross-correlations or both.
baseline-integration         string      Average baselines together to a single dynamic spectrum, with a specified method. Allowed
                                         values are: ``"count"``, ``"average"``, ``"average-abs"``, ``"squared"`` or ``"stddev"``.
chunk-size                   integer     When not zero, ``aoflagger`` will process the data in chunks with the given maximum
                                         chunk size.
column-name                  string      What data column to use, e.g. ``"DATA"``, ``"CORRECTED_DATA"``. etc.
combine-spws                 boolean     Whether to concatenate all spectral windows together while flagging.
execute-file                 string      Name of file to load for this run, which should provide the execute-function. By default,
                                         it is assumed to be the currently loaded file (which also provides the :meth:`options` call).
execute-function             string      Name of function to run (note this is a string, not a function). Default: ``"execute"``.
fields                       table       List of integer (zero-indexed) field ids to process.
files                        table       List of strings that are the names of the files to process.
min‑aoflagger-version        string      Minimum AOFlagger version required, of the form "major.minor". Defaults to ``"3.0"``.
quiet                        boolean     Inhibits all output except errors.
read-mode                    string      ``"direct"``, ``"indirect"``, ``"memory"`` or ``"auto"``.
read-uvws                    boolean     Whether to read the UVWs. This is not done by default.
script-version               string      Version of this strategy. Can have the form "major.minor[.subminor] [extra description]",
                                         for example ``"2.4 beta"`` or ``"3.1.4 modified by André"``.
start-timestep /end-timestep integer     Timestep (zero-indexed) from/at which to start/end processing.
threads                      integer     Number of threads to use. The default is to use one thread per core.
verbose                      boolean     Sets verbose logging.
============================ =======     ===========

