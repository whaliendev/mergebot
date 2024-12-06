export { getSourceLanguage } from "./merge";

export function throttle(func, delay) {
  let lastCallTime = 0;
  let timeoutId;

  return function () {
    const now = Date.now();

    if (now - lastCallTime >= delay) {
      func.apply(this, arguments);
      lastCallTime = now;
    } else {
      clearTimeout(timeoutId);
      timeoutId = setTimeout(
        () => {
          func.apply(this, arguments);
          lastCallTime = now;
        },
        delay - (now - lastCallTime),
      );
    }
  };
}

export function debounce(func, delay) {
  let timeoutId;

  return function () {
    const context = this;
    const args = arguments;

    clearTimeout(timeoutId);
    timeoutId = setTimeout(() => {
      func.apply(context, args);
    }, delay);
  };
}
