#include <concepts>
#include <type_traits>
#include <utility>

namespace hooks {
  template <typename F, typename... Args> struct hook_base {
    using call_t = F;
    using return_t = std::invoke_result_t<F, Args...>;

    F m_old_ptr = nullptr;

    using args_t = std::tuple<Args...>;

    explicit constexpr hook_base(F f) : m_old_ptr(f) {
    }
    
    template <typename HM>
    void enable(HM& hm, F fake_call) {
      m_old_ptr = hm.enable_hook(m_old_ptr, fake_call);
    }

    template <typename HM>
    bool disable(HM& hm) {
      return hm.disable_hook(m_old_ptr);
    }
  };
} // namespace hooks

