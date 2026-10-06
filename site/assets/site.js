const root = document.documentElement;
root.classList.add("js");
const still = matchMedia("(prefers-reduced-motion: reduce)").matches;
const clamp = (v) => Math.min(1, Math.max(0, v));

const observeOnce = (elements, onEnter, margin = "0px 0px -12% 0px") => {
  const io = new IntersectionObserver((entries) => {
    for (const entry of entries) {
      if (!entry.isIntersecting) continue;
      onEnter(entry.target);
      io.unobserve(entry.target);
    }
  }, { rootMargin: margin });
  elements.forEach((el) => io.observe(el));
};

document.querySelectorAll("[data-stagger]").forEach((group) => {
  [...group.children].forEach((child, i) => child.style.setProperty("--i", Math.min(i, 10)));
});
observeOnce(document.querySelectorAll(".fade, .rise"), (el) => el.classList.add("in"));
observeOnce(document.querySelectorAll(".cheats"), (list) => {
  [...list.children].forEach((item, i) => setTimeout(() => item.classList.add("on"), still ? 0 : 260 + i * 170));
}, "0px 0px -30% 0px");

const bar = document.querySelector(".bar");
const tracked = [...document.querySelectorAll("[data-p]")];
const wipes = [...document.querySelectorAll("[data-wipe]")];
const drawn = [...document.querySelectorAll(".steps li")];
const reach = document.querySelector(".reach");
const meter = reach && {
  value: reach.querySelector("[data-meter-value]"),
  angle: reach.querySelector("[data-meter-angle]"),
  label: reach.querySelector(".track .label"),
  track: reach.querySelector(".track"),
  items: [...reach.querySelectorAll(".reach-list li")],
  from: Number(reach.dataset.from),
  to: Number(reach.dataset.to),
  angleFrom: Number(reach.dataset.angleFrom),
  angleTo: Number(reach.dataset.angleTo),
};

const frame = () => {
  const vh = innerHeight;
  const scrollable = root.scrollHeight - vh;
  if (bar) bar.style.setProperty("--read", scrollable > 0 ? (scrollY / scrollable).toFixed(4) : 0);
  for (const el of tracked) {
    const r = el.getBoundingClientRect();
    const p = el.dataset.p === "top" ? clamp(-r.top / (vh * 0.9)) : clamp((vh - r.top) / (vh + r.height));
    el.style.setProperty("--p", p.toFixed(4));
  }
  for (const el of wipes) {
    const r = el.getBoundingClientRect();
    el.style.setProperty("--reveal", clamp((vh - r.top) / (vh * 0.7)).toFixed(4));
    el.style.setProperty("--p", clamp((vh - r.top) / (vh + r.height)).toFixed(4));
  }
  for (const li of drawn) {
    const r = li.getBoundingClientRect();
    li.style.setProperty("--draw", clamp((vh * 0.85 - r.top) / (r.height + vh * 0.2)).toFixed(4));
  }
  if (meter) {
    const r = reach.getBoundingClientRect();
    const p = clamp(-r.top / (r.height - vh));
    reach.style.setProperty("--p", p.toFixed(4));
    reach.style.setProperty("--track-w", `${meter.track.clientWidth}px`);
    meter.value.firstChild.nodeValue = Math.round(meter.from + p * (meter.to - meter.from));
    meter.angle.textContent = `${Math.round(meter.angleFrom + p * (meter.angleTo - meter.angleFrom))}°`;
    meter.label.textContent = p < 0.02 ? "Normal" : p > 0.98 ? "Max" : `${Math.round(p * 100)}%`;
    meter.items.forEach((li, i) => li.classList.toggle("lit", p >= (i + 0.5) / meter.items.length));
  }
};

if (still) {
  wipes.forEach((el) => el.style.setProperty("--reveal", 1));
  if (meter) {
    reach.style.setProperty("--p", 1);
    meter.value.firstChild.nodeValue = meter.to;
    meter.angle.textContent = `${meter.angleTo}°`;
    meter.label.textContent = "Max";
    meter.items.forEach((li) => li.classList.add("lit"));
  }
} else {
  let queued = false;
  const request = () => {
    if (queued) return;
    queued = true;
    requestAnimationFrame(() => { queued = false; frame(); });
  };
  addEventListener("scroll", request, { passive: true });
  addEventListener("resize", request);
  frame();
}

document.querySelectorAll("[data-copy]").forEach((button) => {
  button.addEventListener("click", async () => {
    const text = document.getElementById(button.dataset.copy).textContent;
    try {
      await navigator.clipboard.writeText(text);
      button.textContent = "Copied";
    } catch {
      button.textContent = "Select it";
    }
    setTimeout(() => { button.textContent = "Copy"; }, 1600);
  });
});
