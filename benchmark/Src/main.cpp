

#include "benchmark_types.hpp"
#include "simdjson.h"
#include <yolo-json/parser.hpp>
#include <yolo-json/parser.hpp>
#include <fcntl.h>
#include <iostream>
#include "serializer.hpp"
#include <string>
#include <utxx/time_val.hpp>
#include <vector>
namespace bench
{
template <typename T> std::vector<std::string> GenForParse(int a_sz_mb, T & a_v)
{
  std::vector<std::string> out;
  int ttl_sz = 0;
  std::string json = yjson::SerializeJson<^^T>(a_v);
  for (int i = 0; i < a_sz_mb * 1e6 / json.size() + 1; i++)
  {
    out.push_back(json);
  }

  return out;
}

template <typename T = benchmark_types::LargeDynamic> T GetSample()
{
  return benchmark_types::MakeLargeDynamic(100);
}
template <> benchmark_types::LargeFixed GetSample()
{
  return benchmark_types::MakeLargeFixed();
}
template <> benchmark_types::LargeIgnored GetSample()
{
  return benchmark_types::MakeLargeIgnored();
}
template <> benchmark_types::LargeNotCompressed GetSample()
{
  return benchmark_types::MakeLargeNotCompressed(10);
}
template <> benchmark_types::LargeRandomOrder GetSample()
{
  return benchmark_types::MakeLargeRandomOrder(10);
}
template <> benchmark_types::LargeJust GetSample()
{
  return benchmark_types::MakeLargeJust();
}
template <> benchmark_types::FixedDoubles GetSample()
{
  return benchmark_types::MakeFixedDoubles();
}
template <> benchmark_types::FixedStrings GetSample()
{
  return benchmark_types::MakeFixedStrings();
}
template <> benchmark_types::FixedPoints GetSample()
{
  return benchmark_types::MakeFixedPoints();
}
template <> benchmark_types::FixedInts GetSample()
{
  return benchmark_types::MakeFixedInts();
}
template <> benchmark_types::DynamicInts GetSample()
{
  return benchmark_types::MakeDynamicInts();
}
template <> benchmark_types::DynamicPoints GetSample()
{
  return benchmark_types::MakeDynamicPoints();
}

template <typename T> int RunTestOwn(std::vector<std::string> a_samples)
{
  utxx::time_val ts = utxx::now_utc();
  volatile long sink = 0;
  for (auto & v : a_samples)
  {
    volatile auto r = yjson::detail::ObjectParser::ParseJson<^^T>(v.data(), v.data() + v.size());
    sink += static_cast<long>(*r.first);
  }

  return utxx::now_utc().diff_msec(ts);
}

template <typename T> int RunTestSimd(std::vector<std::string> a_samples)
{
  using namespace simdjson;
  utxx::time_val ts = utxx::now_utc();

  volatile T r;
  for (auto & v : a_samples)
  {
    dom::parser parser;
    simdjson::padded_input input(v);
    volatile dom::element doc = parser.parse(input);
    // volatile ondemand::array items = doc.get_array();
    // ondemand::object object = doc.get_object();
    // for (auto itm : array_)
    // {
    //   std::string_view keyv = itm.unescaped_key();
    //   std::print("{}\n", keyv);
    //   volatile auto v = itm.value();
    // }
  }

  return utxx::now_utc().diff_msec(ts);
}

template <typename T> std::pair<int, int> DoBench(int test_sz_mb)
{
  std::cout << "Doing " << std::meta::display_string_of(^^T) << '\n';
  auto t = GetSample<T>();
  auto samples = bench::GenForParse(test_sz_mb, t);
  std::cout << samples[0] << '\n';
  int time1 = bench::RunTestOwn<T>(samples);
  std::cout << time1 << '\n';
  int time2 = bench::RunTestSimd<T>(samples);
  std::cout << time2 << '\n';

  return {time1, time2};
}
} // namespace bench
int main()
{

  const int test_sz_mb = 500;
  const bool do_large = true;
  const bool do_large_not_comp = true;
  const bool do_fixed = true;
  const bool do_rand = true;
  const bool do_ignore = true;
  const bool do_just = true;
  const bool do_fixed_arrs = true;
  const bool do_dynamic = true;

  if (do_large)
  {
    (void)bench::DoBench<benchmark_types::LargeDynamic>(test_sz_mb);
  }
  if (do_fixed_arrs)
  {
    (void)bench::DoBench<benchmark_types::FixedDoubles>(test_sz_mb);
    (void)bench::DoBench<benchmark_types::FixedStrings>(test_sz_mb);
    (void)bench::DoBench<benchmark_types::FixedPoints>(test_sz_mb);
    (void)bench::DoBench<benchmark_types::FixedInts>(test_sz_mb);
  }
  if (do_dynamic)
  {
    (void)bench::DoBench<benchmark_types::DynamicInts>(test_sz_mb);
    (void)bench::DoBench<benchmark_types::DynamicPoints>(test_sz_mb);
  }
  if (do_large_not_comp)
  {
    (void)bench::DoBench<benchmark_types::LargeNotCompressed>(test_sz_mb);
  }
  if (do_fixed)
  {
    (void)bench::DoBench<benchmark_types::LargeFixed>(test_sz_mb);
  }
  if (do_rand)
  {
    (void)bench::DoBench<benchmark_types::LargeRandomOrder>(test_sz_mb);
  }
  if (do_ignore)
  {
    (void)bench::DoBench<benchmark_types::LargeIgnored>(test_sz_mb);
  }
  if (do_just)
  {
    (void)bench::DoBench<benchmark_types::LargeJust>(test_sz_mb);
  }
  return 0;
}
