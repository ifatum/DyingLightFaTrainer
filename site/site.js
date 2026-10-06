document.documentElement.classList.add("js");

const reveals = document.querySelectorAll("[data-reveal]");
const revealer = new IntersectionObserver((entries) => {
  for (const entry of entries) {
    if (!entry.isIntersecting) continue;
    entry.target.classList.add("in");
    revealer.unobserve(entry.target);
  }
}, { rootMargin: "0px 0px -8% 0px", threshold: 0.08 });
reveals.forEach((el) => revealer.observe(el));

document.querySelectorAll("[data-stagger]").forEach((list) => {
  [...list.children].forEach((child, i) => {
    child.setAttribute("data-reveal", "");
    child.style.setProperty("--i", Math.min(i, 8));
    revealer.observe(child);
  });
});

const rail = document.querySelector(".rail");
if (rail) {
  const marker = rail.querySelector(".marker");
  const links = [...rail.querySelectorAll("a")];
  const byId = new Map(links.map((a) => [a.hash.slice(1), a]));
  const select = (link) => {
    links.forEach((a) => a.classList.toggle("on", a === link));
    marker.style.setProperty("--y", `${link.offsetTop + (link.offsetHeight - marker.offsetHeight) / 2}px`);
    rail.classList.add("ready");
  };
  const spy = new IntersectionObserver((entries) => {
    for (const entry of entries) {
      if (entry.isIntersecting && byId.has(entry.target.id)) select(byId.get(entry.target.id));
    }
  }, { rootMargin: "-35% 0px -60% 0px" });
  byId.forEach((_, id) => {
    const section = document.getElementById(id);
    if (section) spy.observe(section);
  });
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
    setTimeout(() => { button.textContent = "Copy"; }, 1800);
  });
});
