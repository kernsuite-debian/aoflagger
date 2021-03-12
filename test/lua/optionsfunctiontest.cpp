#include <boost/test/unit_test.hpp>
#include <boost/optional/optional_io.hpp>

#include "../../lua/optionsfunction.h"
#include "../../lua/luathreadgroup.h"

BOOST_AUTO_TEST_SUITE(optionsfunction, *boost::unit_test::label("lua"))

BOOST_AUTO_TEST_CASE(no_function) {
  LuaThreadGroup lua(1);
  lua.LoadText("-- nothing here\n");
  std::map<std::string, Options> options =
      OptionsFunction::GetOptions(lua.GetThread(0).State(), Options());
  BOOST_CHECK(options.empty());
}

BOOST_AUTO_TEST_CASE(empty) {
  LuaThreadGroup lua(1);
  lua.LoadText(
      "function options()\n"
      "  return { }\n"
      "end\n");
  std::map<std::string, Options> options =
      OptionsFunction::GetOptions(lua.GetThread(0).State(), Options());
  BOOST_CHECK(options.empty());
}

BOOST_AUTO_TEST_CASE(single_default_run) {
  LuaThreadGroup lua(1);
  lua.LoadText(
      "function options()\n"
      "  mainopts = { }\n"
      "  runs = { }\n"
      "  runs[\"main\"] = mainopts\n"
      "  return runs\n"
      "end\n");
  std::map<std::string, Options> options =
      OptionsFunction::GetOptions(lua.GetThread(0).State(), Options());
  BOOST_REQUIRE_EQUAL(options.size(), 1);
  BOOST_CHECK_EQUAL(options.begin()->first, "main");
  BOOST_CHECK(options.begin()->second == Options());
}

BOOST_AUTO_TEST_CASE(single_dual_run) {
  LuaThreadGroup lua(1);
  lua.LoadText(
      "function options()\n"
      "  first_opts = { }\n"
      "  second_opts = { }\n"
      "  second_opts.bands = { 11, 13 }\n"
      "  second_opts.baselines = \"auto\"\n"
      "  second_opts[\"chunk-size\"] = 3\n"
      "  second_opts[\"column-name\"] = \"MY_DATA\"\n"
      "  second_opts[\"combine-spws\"] = true\n"
      "  second_opts[\"execute-file\"] = \"somefile.lua\"\n"
      "  second_opts[\"execute-function\"] = \"my_execute\"\n"
      "  second_opts.fields = { 3, 4, 5 }\n"
      "  second_opts.files = { \"first_file.txt\", \"second_file.txt\" }\n"
      "  second_opts[\"min‑aoflagger-version\"] = \"2.99\"\n"
      "  second_opts.quiet = true\n"
      "  second_opts[\"read-mode\"] = \"direct\"\n"
      "  second_opts[\"read-uvws\"] = false\n"
      "  second_opts[\"script-version\"] = \"1.0\"\n"
      "  second_opts[\"start-timestep\"] = 1982\n"
      "  second_opts[\"end-timestep\"] = 2020\n"
      "  second_opts.threads = 24\n"
      "  second_opts.verbose = true\n"
      "  runs = { }\n"
      "  runs[\"first\"] = first_opts\n"
      "  runs[\"second\"] = second_opts\n"
      "  return runs\n"
      "end\n");
  std::map<std::string, Options> options =
      OptionsFunction::GetOptions(lua.GetThread(0).State(), Options());
  BOOST_REQUIRE_EQUAL(options.size(), 2);

  Options secOptions;
  secOptions.bands = std::set<size_t>{11, 13};
  secOptions.baselineSelection = BaselineSelection::AutoCorrelations;
  secOptions.chunkSize = 3;
  secOptions.dataColumn = "MY_DATA";
  secOptions.combineSPWs.emplace(true);
  secOptions.executeFilename = "somefile.lua";
  secOptions.executeFunctionName = "my_execute";
  secOptions.fields = std::set<size_t>{3, 4, 5};
  secOptions.filenames =
      std::vector<std::string>{"first_file.txt", "second_file.txt"};
  secOptions.logVerbosity =
      Logger::QuietVerbosity;  // be aware that verbose = true is overruled by
                               // quiet = true
  secOptions.readMode = BaselineIOMode::DirectReadMode;
  secOptions.readUVW = false;
  secOptions.scriptVersion = "1.0";
  secOptions.startTimestep = 1982;
  secOptions.endTimestep = 2020;
  secOptions.threadCount = 24;

  BOOST_CHECK_EQUAL(options.begin()->first, "first");
  BOOST_CHECK(options.begin()->second == Options());
  BOOST_CHECK(options.begin()->second != secOptions);

  BOOST_CHECK_EQUAL(options.rbegin()->first, "second");
  BOOST_CHECK_EQUAL(options.rbegin()->second.chunkSize, secOptions.chunkSize);
  BOOST_CHECK_EQUAL(options.rbegin()->second.dataColumn, secOptions.dataColumn);
  BOOST_CHECK_EQUAL(options.rbegin()->second.combineSPWs,
                    secOptions.combineSPWs);
  BOOST_CHECK_EQUAL(options.rbegin()->second.readMode, secOptions.readMode);
  BOOST_CHECK_EQUAL(options.rbegin()->second.readUVW, secOptions.readUVW);
  BOOST_CHECK_EQUAL(options.rbegin()->second.scriptVersion,
                    secOptions.scriptVersion);
  BOOST_CHECK_EQUAL(options.rbegin()->second.startTimestep,
                    secOptions.startTimestep);
  BOOST_CHECK_EQUAL(options.rbegin()->second.endTimestep,
                    secOptions.endTimestep);
  BOOST_CHECK(options.rbegin()->second == secOptions);
  BOOST_CHECK(options.rbegin()->second != Options());
}

BOOST_AUTO_TEST_CASE(version_check) {
  LuaThreadGroup lua(1);
  lua.LoadText(
      "function options()\n"
      "  mainopts = { }\n"
      "  mainopts[\"min‑aoflagger-version\"] = \"999.999\"\n"
      "  runs = { }\n"
      "  runs[\"main\"] = mainopts\n"
      "  return runs\n"
      "end\n");
  BOOST_CHECK_THROW(
      OptionsFunction::GetOptions(lua.GetThread(0).State(), Options()),
      std::exception);
}

BOOST_AUTO_TEST_CASE(cmdline_override) {
  LuaThreadGroup lua(1);
  lua.LoadText(
      "function options()\n"
      "  opt_a = { }\n"
      "  opt_a[\"column-name\"] = \"option\"\n"
      "  opt_b = { }\n"
      "  opt_b[\"baselines\"] = \"auto\"\n"
      "  opt_c = { }\n"
      "  opt_c[\"threads\"] = 16533\n"
      "  opt_d = { }\n"
      "  opt_d[\"verbose\"] = false\n"
      "  return { opt_a, opt_b, opt_c, opt_d }\n"
      "end\n");
  Options cmdLineOptions;
  cmdLineOptions.dataColumn = "override_column";
  cmdLineOptions.baselineSelection = BaselineSelection::All;
  cmdLineOptions.threadCount = 2;
  cmdLineOptions.logVerbosity = Logger::VerboseVerbosity;

  std::map<std::string, Options> options =
      OptionsFunction::GetOptions(lua.GetThread(0).State(), cmdLineOptions);

  BOOST_REQUIRE_EQUAL(options.size(), 4);
  for (const auto& x : options) {
    BOOST_CHECK(x.second == cmdLineOptions);
  }
}

BOOST_AUTO_TEST_SUITE_END()
