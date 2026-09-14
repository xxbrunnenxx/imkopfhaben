(function(){const t=document.createElement("link").relList;if(t&&t.supports&&t.supports("modulepreload"))return;for(const o of document.querySelectorAll('link[rel="modulepreload"]'))i(o);new MutationObserver(o=>{for(const a of o)if(a.type==="childList")for(const l of a.addedNodes)l.tagName==="LINK"&&l.rel==="modulepreload"&&i(l)}).observe(document,{childList:!0,subtree:!0});function n(o){const a={};return o.integrity&&(a.integrity=o.integrity),o.referrerPolicy&&(a.referrerPolicy=o.referrerPolicy),o.crossOrigin==="use-credentials"?a.credentials="include":o.crossOrigin==="anonymous"?a.credentials="omit":a.credentials="same-origin",a}function i(o){if(o.ep)return;o.ep=!0;const a=n(o);fetch(o.href,a)}})();const Mt={mode:"absolute",position:"bottom",height:"6rem",strength:2,divCount:5,exponential:!1,opacity:1,curve:"linear",zIndex:2},it={linear:e=>e,bezier:e=>e*e*(3-2*e),"ease-in":e=>e*e,"ease-out":e=>1-Math.pow(1-e,2),"ease-in-out":e=>e<.5?2*e*e:1-Math.pow(-2*e+2,2)/2};function zt(e){switch(e){case"top":return"to top";case"left":return"to left";case"right":return"to right";default:return"to bottom"}}function qt(){if(typeof CSS>"u"||typeof CSS.supports!="function")return!1;const e=CSS.supports("backdrop-filter","blur(1px)")||CSS.supports("-webkit-backdrop-filter","blur(1px)"),t=CSS.supports("mask-image","linear-gradient(to bottom, transparent, black)")||CSS.supports("-webkit-mask-image","linear-gradient(to bottom, transparent, black)");return e&&t}function Nt(e){const t={...Mt,...e},n=document.createElement("div"),i=document.createElement("div"),o=zt(t.position),a=qt(),l=t.position==="top"||t.position==="bottom";if(n.className="gradual-blur",t.className&&n.classList.add(t.className),a||(n.classList.add("gradual-blur--fallback"),n.style.background=`linear-gradient(${o}, transparent 0%, ${t.fallbackColor||"rgba(255, 255, 255, 0.95)"} 100%)`),n.style.position=t.mode,n.style.pointerEvents="none",n.style.zIndex=String(t.zIndex),n.style.opacity="0",n.style.borderRadius="inherit",l?(n.style.height=t.height,n.style.width=t.width||"100%",n.style[t.position]="0",t.mode==="fixed"?(n.style.left="50%",n.style.right="auto",n.style.transform="translateX(-50%)"):(n.style.left="0",n.style.right="0")):(n.style.width=t.width||t.height,n.style.height="100%",n.style.top="0",n.style.bottom="0",n.style[t.position]="0"),i.className="gradual-blur__inner",n.appendChild(i),a){const c=100/t.divCount,u=it[t.curve]||it.linear;for(let f=1;f<=t.divCount;f+=1){const y=document.createElement("div"),E=u(f/t.divCount);let S=0;t.exponential?S=Math.pow(2,E*4)*.0625*t.strength:S=.0625*(E*t.divCount+1)*t.strength;const A=Math.round((c*f-c)*10)/10,C=Math.round(c*f*10)/10,x=Math.round((c*f+c)*10)/10,k=Math.round((c*f+c*2)*10)/10;let s=`transparent ${A}%, black ${C}%`;x<=100&&(s+=`, black ${x}%`),k<=100&&(s+=`, transparent ${k}%`),y.className="gradual-blur__layer",y.style.maskImage=`linear-gradient(${o}, ${s})`,y.style.webkitMaskImage=`linear-gradient(${o}, ${s})`,y.style.backdropFilter=`blur(${S.toFixed(3)}rem)`,y.style.setProperty("-webkit-backdrop-filter",`blur(${S.toFixed(3)}rem)`),y.style.opacity=String(t.opacity),i.appendChild(y)}}return window.getComputedStyle(t.mount).position==="static"&&(t.mount.style.position="relative"),t.mount.appendChild(n),{element:n,setVisible(c){n.style.opacity=c?"1":"0"},destroy(){n.remove()}}}const Pt=`:host {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 1.75rem;
  padding: 0 var(--spacing-sm);
  border: var(--border-01) solid var(--color-border);
  border-radius: var(--radius-full);
  background-color: var(--color-bg-selected);
  color: var(--color-text-secondary);
  font-size: var(--font-size-caption);
  font-weight: var(--font-weight-semibold);
  line-height: 1;
  white-space: nowrap;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

.badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  white-space: inherit;
}
`,Vt=document.createElement("template");Vt.innerHTML=`
  <style>${Pt}</style>
  <span class="badge">
    <slot></slot>
  </span>
`;const Ot=`:host {
  display: inline-flex;
  height: 44px;
  width: auto;
  min-width: 96px;
  vertical-align: middle;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

button {
  all: unset;
  box-sizing: border-box;
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  height: 44px;
  width: auto;
  min-height: 44px;
  min-width: 96px;
  padding: var(--spacing-3xs) var(--spacing-sm);
  border: var(--border-01) solid transparent;
  border-radius: var(--button-radius);
  background-color: var(--color-text-secondary);
  color: var(--color-text-inverse);
  cursor: pointer;
  font-family: var(--font-family);
  font-size: var(--font-size-label);
  font-weight: var(--font-weight-regular);
  line-height: 1.25;
  text-align: center;
}

button:focus-visible {
  outline: var(--focus-ring-width) solid var(--focus-ring-color);
  outline-offset: var(--focus-ring-offset);
}

button:disabled {
  background-color: var(--color-bg-mute);
  color: var(--color-text-mute);
  cursor: default;
}

:host([variant="primary"]) button {
  background-color: var(--color-bg-brand);
  color: var(--color-text-inverse);
}

:host([variant="primary"]) button:not(:disabled):is(:hover, :active) {
  background-color: var(--color-bg-brand-hover);
}

:host([variant="secondary"]) button {
  background-color: var(--color-bg-active);
  color: var(--color-fg-active);
}

:host([variant="secondary"]) button:not(:disabled):is(:hover, :active) {
  background-color: var(--color-bg-active);
  color: var(--color-fg-active);
}

:host([variant="outline"]) button {
  background-color: transparent;
  border-color: var(--color-border);
  color: var(--color-text-secondary);
}

:host([variant="outline"]) button:not(:disabled):is(:hover, :active) {
  background-color: var(--color-border);
  border-color: var(--color-border);
}

:host([variant="ghost"]) button {
  background-color: transparent;
  border-color: transparent;
  color: var(--color-text-secondary);
}

:host([variant="ghost"]) button:not(:disabled):is(:hover, :active) {
  background-color: var(--color-overlay);
}

:host([inverse]) button {
  color: var(--color-text-inverse);
}

:host([variant="outline"][inverse]) button {
  border-color: var(--color-text-inverse);
}

:host([variant="outline"][inverse]) button:not(:disabled):is(:hover, :active),
:host([variant="ghost"][inverse]) button:not(:disabled):is(:hover, :active) {
  background-color: var(--color-overlay);
  border-color: var(--color-text-inverse);
}

:host([rounded]) button {
  border-radius: var(--radius-full);
  padding: var(--spacing-xs) var(--spacing-lg);
}

::slotted(svg) {
  flex-shrink: 0;
}
`,R="ui-button",ct=document.createElement("template");ct.innerHTML=`
  <style>${Ot}</style>
  <button type="button" part="button">
    <slot></slot>
  </button>
`;function ot(e){return e==="submit"||e==="reset"?e:"button"}class Dt extends HTMLElement{static get observedAttributes(){return["disabled","aria-label","type"]}constructor(){super();const t=this.attachShadow({mode:"open",delegatesFocus:!0});t.append(ct.content.cloneNode(!0)),this.button=t.querySelector("button")}connectedCallback(){this.dataset.component=R,this.syncAttributes()}attributeChangedCallback(){this.syncAttributes()}get disabled(){return this.hasAttribute("disabled")}set disabled(t){this.toggleAttribute("disabled",t)}get type(){return ot(this.getAttribute("type"))}set type(t){this.setAttribute("type",ot(t))}focus(t){this.button.focus(t)}click(){this.button.click()}syncAttributes(){this.button.disabled=this.disabled,this.button.type=this.type,this.tabIndex=this.disabled?-1:0;const t=this.getAttribute("aria-label");t?this.button.setAttribute("aria-label",t):this.button.removeAttribute("aria-label")}}function Ht(){customElements.get(R)||customElements.define(R,Dt)}const $t=`:host {
  display: contents;
}

:host([hidden]) {
  display: none;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

.bottom-sheet-dialog {
  position: fixed;
  inset: auto 0 0 0;
  display: flex;
  width: 100%;
  max-height: 96vh;
  padding: 0;
  border: 0;
  background: transparent;
  color: var(--color-text);
  overflow: visible;
  transform: translateY(100%);
  max-width: 768px;
  margin: auto;
}

.bottom-sheet-dialog:not([open]) {
  pointer-events: none;
  visibility: hidden;
}

.bottom-sheet-dialog[open] {
  animation: bottom-sheet-open 320ms ease forwards;
}

.bottom-sheet-dialog.closing {
  animation: bottom-sheet-close 320ms ease forwards;
}

.bottom-sheet-dialog::backdrop {
  background: var(--color-overlay);
  backdrop-filter: blur(4px) saturate(180%);
  -webkit-backdrop-filter: blur(4px) saturate(180%);
}

.bottom-sheet-content {
  display: flex;
  flex-direction: column;
  width: 100%;
  min-height: 0;
  border-top: var(--border-01) solid var(--color-border);
  border-radius: var(--radius-xl) var(--radius-xl) 0 0;
  background: var(--color-bg);
  box-shadow: 0 -8px 24px rgb(0 0 0 / 0.16);
  overflow: hidden;
}

.bottom-sheet-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: var(--spacing-sm);
  padding: var(--spacing-sm) var(--spacing-md);
  padding-right: var(--spacing-sm);
  border-bottom: var(--border-01) solid var(--color-border);
}

.bottom-sheet-header-copy {
  display: grid;
  gap: var(--spacing-3xs);
  min-width: 0;
}

.bottom-sheet-title,
.bottom-sheet-description {
  margin: 0;
}

.bottom-sheet-title {
  font-size: var(--font-size-heading);
  font-weight: var(--font-weight-semibold);
  line-height: 1.2;
}

.bottom-sheet-description {
  color: var(--color-text-secondary);
  font-size: var(--font-size-label);
  line-height: 1.5;
}

.bottom-sheet-header-actions {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: var(--spacing-xs);
  flex-shrink: 0;
}

.bottom-sheet-body {
  display: flex;
  flex-direction: column;
  gap: var(--spacing-sm);
  min-height: 0;
  padding: var(--spacing-md);
  overflow: auto;
}

.bottom-sheet-footer {
  display: flex;
  flex-wrap: wrap;
  justify-content: flex-end;
  gap: var(--spacing-sm);
  /* Extra bottom padding so the footer buttons clear the iOS home indicator / safe area and
     aren't clipped at the bottom of the sheet. */
  padding: var(--spacing-md) var(--spacing-md) var(--spacing-6xl);
  border-top: var(--border-01) solid var(--color-border);
  background: var(--color-bg);
}

.bottom-sheet-title[hidden],
.bottom-sheet-description[hidden],
.bottom-sheet-header-actions[hidden],
.bottom-sheet-footer[hidden] {
  display: none;
}

::slotted([slot="footer"]) {
  flex-shrink: 0;
}

@keyframes bottom-sheet-open {
  from {
    transform: translateY(100%);
  }

  to {
    transform: translateY(0);
  }
}

@keyframes bottom-sheet-close {
  from {
    transform: translateY(0);
  }

  to {
    transform: translateY(100%);
  }
}
`,F="ui-bottom-sheet",dt=document.createElement("template"),Rt=["a[href]","button:not([disabled])","details summary","input:not([disabled])","select:not([disabled])","textarea:not([disabled])",'[tabindex]:not([tabindex="-1"])'].join(", ");let D=0;dt.innerHTML=`
  <style>${$t}</style>
  <dialog class="bottom-sheet-dialog" part="dialog">
    <div class="bottom-sheet-content" part="content">
      <div class="bottom-sheet-header" part="header">
        <div class="bottom-sheet-header-copy">
          <h2 class="bottom-sheet-title" part="title"></h2>
          <p class="bottom-sheet-description" part="description"></p>
        </div>
        <div class="bottom-sheet-header-actions" part="header-actions">
          <slot name="header-actions"></slot>
        </div>
      </div>
      <div class="bottom-sheet-body" part="body">
        <slot></slot>
      </div>
      <div class="bottom-sheet-footer" part="footer">
        <slot name="footer"></slot>
      </div>
    </div>
    <div class="bottom-sheet-floating" part="floating-content">
      <slot name="floating-content"></slot>
    </div>
  </dialog>
`;function rt(e){const t=getComputedStyle(e);return e.hasAttribute("hidden")||e.getAttribute("aria-hidden")==="true"||t.display==="none"||t.visibility==="hidden"?!1:e.tagName.includes("-")?t.display==="contents"||!!e.getClientRects().length:e.offsetParent!==null||t.position==="fixed"||!!e.getClientRects().length}function H(e){if(!e.shadowRoot)return[];const t=[],n=new Set;function i(o){if(o instanceof HTMLSlotElement){const a=o.assignedElements({flatten:!0});a.length>0?a.forEach(i):Array.from(o.children).forEach(i);return}if(o instanceof HTMLElement&&rt(o)){if(o.shadowRoot){Array.from(o.shadowRoot.childNodes).forEach(i);return}!n.has(o)&&o.matches(Rt)&&rt(o)&&(n.add(o),t.push(o)),Array.from(o.children).forEach(i)}}return Array.from(e.shadowRoot.childNodes).forEach(i),t}function Ft(e,t){if(t===e||t.contains(e))return!0;let n=e;for(;n;){if(n===t)return!0;const i=n.getRootNode();if(!(i instanceof ShadowRoot))break;n=i.host}return!1}function $(e){return e.assignedNodes({flatten:!0}).some(t=>t.nodeType===Node.TEXT_NODE?t.textContent?.trim().length:t.nodeType===Node.ELEMENT_NODE)}class Ut extends HTMLElement{constructor(){super(),this.isAnimatingClose=!1,this.isSyncingOpenAttribute=!1,this.previousDocumentOverflow="",this.previousBodyOverflow="",this.triggerElement=null;const t=this.attachShadow({mode:"open"});t.append(dt.content.cloneNode(!0)),this.dialog=t.querySelector(".bottom-sheet-dialog"),this.titleElement=t.querySelector(".bottom-sheet-title"),this.descriptionElement=t.querySelector(".bottom-sheet-description"),this.headerActionsContainer=t.querySelector(".bottom-sheet-header-actions"),this.footerContainer=t.querySelector(".bottom-sheet-footer"),this.floatingContainer=t.querySelector(".bottom-sheet-floating"),this.headerActionsSlot=t.querySelector('slot[name="header-actions"]'),this.footerSlot=t.querySelector('slot[name="footer"]'),this.floatingSlot=t.querySelector('slot[name="floating-content"]'),D+=1,this.titleId=`bottomSheetTitle${D}`,this.descriptionId=`bottomSheetDescription${D}`,this.handleAnimationEndBound=this.handleAnimationEnd.bind(this),this.handleCancelBound=this.handleCancel.bind(this),this.handleClickBound=this.handleClick.bind(this),this.handleKeyDownBound=this.handleKeyDown.bind(this),this.handleSlotChangeBound=this.syncSlotVisibility.bind(this)}static get observedAttributes(){return["description","open","title"]}connectedCallback(){this.dataset.component=F,this.dialog.addEventListener("animationend",this.handleAnimationEndBound),this.dialog.addEventListener("cancel",this.handleCancelBound),this.dialog.addEventListener("click",this.handleClickBound),this.dialog.addEventListener("keydown",this.handleKeyDownBound),this.addEventListener("keydown",this.handleKeyDownBound),this.headerActionsSlot.addEventListener("slotchange",this.handleSlotChangeBound),this.footerSlot.addEventListener("slotchange",this.handleSlotChangeBound),this.floatingSlot.addEventListener("slotchange",this.handleSlotChangeBound),this.syncAttributes(),this.syncSlotVisibility(),this.hasAttribute("open")&&this.openInternal()}disconnectedCallback(){this.dialog.removeEventListener("animationend",this.handleAnimationEndBound),this.dialog.removeEventListener("cancel",this.handleCancelBound),this.dialog.removeEventListener("click",this.handleClickBound),this.dialog.removeEventListener("keydown",this.handleKeyDownBound),this.removeEventListener("keydown",this.handleKeyDownBound),this.headerActionsSlot.removeEventListener("slotchange",this.handleSlotChangeBound),this.footerSlot.removeEventListener("slotchange",this.handleSlotChangeBound),this.floatingSlot.removeEventListener("slotchange",this.handleSlotChangeBound),this.dialog.open&&this.dialog.close(),this.unlockDocumentScroll()}attributeChangedCallback(t){if(t==="open"){if(!this.isConnected||this.isSyncingOpenAttribute)return;this.hasAttribute("open")?this.openInternal():this.startClosing();return}this.syncAttributes()}get open(){return this.hasAttribute("open")}set open(t){this.toggleAttribute("open",t)}openBottomSheet(){this.open&&!this.isAnimatingClose||(this.triggerElement=document.activeElement instanceof HTMLElement?document.activeElement:null,this.open=!0)}closeBottomSheet(){!this.open&&!this.dialog.open||(this.open=!1)}focus(t){(H(this)[0]??this.dialog).focus(t)}syncAttributes(){const t=this.getAttribute("title")||"",n=this.getAttribute("description")||"";this.titleElement.id=this.titleId,this.titleElement.textContent=t,this.titleElement.hidden=t.length===0,this.descriptionElement.id=this.descriptionId,this.descriptionElement.textContent=n,this.descriptionElement.hidden=n.length===0,t.length>0?this.dialog.setAttribute("aria-labelledby",this.titleId):this.dialog.removeAttribute("aria-labelledby"),n.length>0?this.dialog.setAttribute("aria-describedby",this.descriptionId):this.dialog.removeAttribute("aria-describedby")}syncSlotVisibility(){this.headerActionsContainer.hidden=!$(this.headerActionsSlot),this.footerContainer.hidden=!$(this.footerSlot),this.floatingContainer.hidden=!$(this.floatingSlot)}handleAnimationEnd(t){!this.isAnimatingClose||t.target!==this.dialog||(this.dialog.classList.remove("closing"),this.dialog.close(),this.isAnimatingClose=!1,this.unlockDocumentScroll(),this.restoreTriggerFocus(),this.dispatchEvent(new Event("close",{bubbles:!0,composed:!0})))}handleCancel(t){t.preventDefault(),this.closeBottomSheet()}handleClick(t){this.isAnimatingClose||t.target===this.dialog&&this.closeBottomSheet()}handleKeyDown(t){if(t.currentTarget===this&&t.composedPath().includes(this.dialog))return;if(t.key==="Escape"){t.preventDefault(),this.closeBottomSheet();return}if(t.key!=="Tab")return;const n=H(this);if(n.length===0){t.preventDefault(),this.dialog.focus();return}const i=t.composedPath()[0],o=i instanceof HTMLElement?i:document.activeElement instanceof HTMLElement?document.activeElement:null,a=o===null?-1:n.findIndex(f=>Ft(o,f)),l=t.shiftKey?-1:1,c=t.shiftKey?n.length-1:0,u=a===-1?c:(a+l+n.length)%n.length;t.preventDefault(),n[u].focus()}openInternal(){this.syncAttributes(),this.syncSlotVisibility(),this.isAnimatingClose&&(this.dialog.classList.remove("closing"),this.isAnimatingClose=!1),this.dialog.open?this.lockDocumentScroll():(this.lockDocumentScroll(),this.dialog.showModal()),requestAnimationFrame(()=>{if(!this.dialog.open)return;(H(this)[0]??this.dialog).focus()})}startClosing(){!this.dialog.open||this.isAnimatingClose||(this.dialog.classList.add("closing"),this.isAnimatingClose=!0)}lockDocumentScroll(){this.previousDocumentOverflow.length===0&&this.previousBodyOverflow.length===0&&(this.previousDocumentOverflow=document.documentElement.style.overflow,this.previousBodyOverflow=document.body.style.overflow),document.documentElement.style.overflow="hidden",document.body.style.overflow="hidden"}unlockDocumentScroll(){document.documentElement.style.overflow=this.previousDocumentOverflow,document.body.style.overflow=this.previousBodyOverflow,this.previousDocumentOverflow="",this.previousBodyOverflow=""}restoreTriggerFocus(){this.triggerElement&&document.contains(this.triggerElement)&&this.triggerElement.focus(),this.triggerElement=null}}function Zt(){customElements.get(F)||customElements.define(F,Ut)}const Wt=`:host {
  display: block;
  width: 100%;
}

:host([hidden]) {
  display: none;
  visibility: hidden;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

h2 {
  margin: 0;
}

.card-component {
  background-color: var(--color-bg-input);
  border-radius: var(--radius-lg);
  border: var(--border-01) solid var(--color-border);
  box-shadow: none;
}

.card-component__container {
  position: relative;
  display: flex;
  flex-direction: column;
  width: 100%;
  background-color: var(--color-bg);
  border-radius: inherit;
  padding: var(--spacing-lg);
  box-shadow: none;
}

.card-component__header {
  position: relative;
  display: flex;
  align-items: center;
  width: 100%;
  margin-bottom: var(--spacing-sm);
}

.card-icon {
  display: flex;
  align-items: center;
  justify-content: center;
}

.card-component__icon {
  width: 24px;
  height: 24px;
  fill: var(--color-text-secondary);
  margin-right: var(--spacing-xs);
}

.card-component__title {
  font-size: var(--font-size-heading);
  font-weight: var(--font-weight-semibold);
  margin: 0;
}

.card-component__content {
  position: relative;
  display: flex;
  flex-direction: column;
  width: 100%;
  margin-top: var(--spacing-xs);
  gap: var(--spacing-md);
}

.connection-notification {
  position: relative;
  display: flex;
  width: 100%;
  flex-direction: column;
  margin-bottom: var(--spacing-sm);
  color: var(--color-text-secondary);
  font-size: var(--font-size-label);
}

.connection-notification[hidden] {
  display: none;
}

.connection-notification[data-status="warning"] {
  color: var(--color-text-warning);
}

.connection-notification[data-status="success"] {
  color: var(--color-meter-green);
}

.connection-notification[data-status="error"],
.connection-notification[data-status="danger"] {
  color: var(--color-text-danger);
}

.card-actions {
  margin-top: var(--spacing-xl);
}

.wifi-connection-form__actions {
  position: relative;
  display: flex;
  align-items: center;
  width: 100%;
  gap: var(--spacing-sm);
}

.card-component__footer[hidden] {
  display: none;
}

::slotted(.card-component__description) {
  font-size: var(--font-size-label);
  line-height: 1.5;
  margin-bottom: var(--spacing-md);
  color: var(--color-text-secondary);
}

::slotted([slot="footer"]) {
  width: auto;
}
`,U="ui-card",ut=document.createElement("template");ut.innerHTML=`
  <style>${Wt}</style>
  <div class="card-component">
    <div class="card-component__container">
      <div class="card-component__header">
        <span class="card-icon">
          <span class="card-component__icon"></span>
        </span>
        <h2 class="card-component__title"></h2>
      </div>
      <div
        class="connection-notification"
        role="status"
        aria-live="polite"
        aria-atomic="true"
        hidden
      ></div>
      <div class="card-component__content">
        <slot></slot>
      </div>
      <div class="card-actions wifi-connection-form__actions card-component__footer">
        <slot name="footer"></slot>
      </div>
    </div>
  </div>
`;function Jt(e,t){return e.includes("class=")?e.replace("<svg",`<svg class="${t}"`):e.replace("<svg",`<svg class="${t}"`)}class Kt extends HTMLElement{static get observedAttributes(){return["title"]}constructor(){super();const t=this.attachShadow({mode:"open"});t.append(ut.content.cloneNode(!0)),this.footerContainer=t.querySelector(".card-component__footer"),this.footerSlot=t.querySelector('slot[name="footer"]'),this.footerSlot.addEventListener("slotchange",()=>{this.syncFooterVisibility()}),this.iconElement=t.querySelector(".card-component__icon"),this.notificationElement=t.querySelector(".connection-notification"),this.titleElement=t.querySelector(".card-component__title")}connectedCallback(){this.dataset.component=U,this.syncAttributes(),this.syncFooterVisibility()}attributeChangedCallback(){this.syncAttributes()}get iconSvg(){return this.iconElement.innerHTML}set iconSvg(t){this.iconElement.innerHTML=t?Jt(t,"card-component__icon"):""}setNotification(t,n="info"){if(!t){this.notificationElement.hidden=!0,this.notificationElement.textContent="",this.notificationElement.removeAttribute("data-status"),this.notificationElement.setAttribute("role","status"),this.notificationElement.setAttribute("aria-live","polite");return}this.notificationElement.hidden=!1,this.notificationElement.textContent=t,this.notificationElement.setAttribute("data-status",n),n==="error"||n==="danger"?(this.notificationElement.setAttribute("role","alert"),this.notificationElement.setAttribute("aria-live","assertive")):n==="warning"?(this.notificationElement.setAttribute("role","alert"),this.notificationElement.setAttribute("aria-live","polite")):(this.notificationElement.setAttribute("role","status"),this.notificationElement.setAttribute("aria-live","polite"))}syncAttributes(){this.titleElement.textContent=this.getAttribute("title")||""}syncFooterVisibility(){const t=this.footerSlot.assignedElements({flatten:!0}).length>0;this.footerContainer.hidden=!t}}function Gt(){customElements.get(U)||customElements.define(U,Kt)}const jt=`:host {
  display: block;
  width: 100%;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

.form-control {
  position: relative;
  display: flex;
  flex-direction: column;
  width: 100%;
}

.form-control label {
  font-size: var(--font-size-label);
  font-weight: var(--font-weight-regular);
  margin-bottom: var(--spacing-xs);
}

.form-control__input-wrapper {
  position: relative;
  display: flex;
  align-items: center;
  width: 100%;
  gap: var(--spacing-sm);
}

.duration-fields {
  align-items: center;
}

.duration-fields ui-input {
  min-width: 0;
  flex: 1 1 0;
}
`,Yt=document.createElement("template");Yt.innerHTML=`
  <style>${jt}</style>
  <div class="form-control">
    <label></label>
    <div class="form-control__input-wrapper duration-fields">
      <ui-input variant="number" inputmode="numeric"></ui-input>
      <ui-input variant="number" inputmode="numeric"></ui-input>
    </div>
  </div>
`;const Xt=`:host {
  display: block;
  width: 100%;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

input,
textarea {
  margin: 0;
  font: inherit;
  color: inherit;
  -webkit-user-select: auto;
}

p {
  margin: 0;
}

.form-control {
  position: relative;
  display: flex;
  flex-direction: column;
  width: 100%;
}

.form-control label {
  font-size: var(--font-size-label);
  font-weight: var(--font-weight-regular);
  margin-bottom: var(--spacing-xs);
}

.form-control__input-wrapper {
  position: relative;
  display: flex;
  align-items: center;
  width: 100%;
  gap: var(--spacing-sm);
}

.form-control__input-wrapper input {
  position: relative;
  display: flex;
  align-items: center;
  width: 100%;
  border: var(--border-01) solid var(--color-border);
  min-height: 44px;
  padding: var(--spacing-sm);
  border-radius: var(--input-radius);
  background-color: var(--color-bg-input);
  color: var(--color-text);
  font-size: var(--font-size-label);
}

.form-control__input-wrapper input:focus-visible {
  outline: var(--focus-ring-width) solid var(--focus-ring-color);
  outline-offset: var(--focus-ring-offset);
}

.form-control__input-wrapper input::file-selector-button {
  background-color: transparent;
  border: var(--border-01) solid var(--color-border);
  background-color: var(--color-bg);
  color: var(--color-text);
  border-radius: var(--radius-sm);
  min-height: 24px;
  font-size: var(--font-size-label);
  padding: var(--spacing-xs) var(--spacing-md);
  margin-right: var(--spacing-md);
}

.progress-bar {
  position: relative;
  width: 100%;
}

.progress-bar[hidden] {
  display: none;
}

.progress-bar progress[value] {
  -webkit-appearance: none;
  appearance: none;
  border: none;
  width: 100%;
  height: 12px;
  border-radius: var(--radius-lg);
  overflow: hidden;
  margin-top: var(--spacing-sm);
}

.progress-bar progress[value]::-webkit-progress-bar {
  background-color: var(--color-bg-active);
  border-radius: var(--radius-lg);
}

.progress-bar progress[value]::-webkit-progress-value {
  background-color: var(--color-meter-green);
  border-radius: var(--radius-lg);
}

.progress-bar__message {
  font-size: var(--font-size-label);
  line-height: 1.25;
  color: var(--color-text-secondary);
  margin-top: var(--spacing-xs);
}

.form-helper-text {
  margin-top: var(--spacing-lg);
  font-size: var(--font-size-caption);
  line-height: 1.5;
  color: var(--color-text-secondary);
}
`,Qt=document.createElement("template");Qt.innerHTML=`
  <style>${Xt}</style>
  <div class="form-control">
    <label></label>
    <div class="form-control__input-wrapper">
      <input type="file" />
    </div>
    <div class="progress-bar">
      <progress value="0" max="100"></progress>
      <div class="progress-bar__message"></div>
    </div>
    <p class="form-helper-text"></p>
  </div>
`;const te=`:host {
  display: inline-flex;
  width: 44px;
  height: 44px;
  min-width: 44px;
  vertical-align: middle;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

button {
  all: unset;
  box-sizing: border-box;
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 44px;
  height: 44px;
  min-width: 44px;
  min-height: 44px;
  padding: 0;
  border: var(--border-01) solid transparent;
  border-radius: var(--button-radius);
  background-color: var(--color-text-secondary);
  color: var(--color-text-inverse);
  cursor: pointer;
}

button:focus-visible {
  outline: var(--focus-ring-width) solid var(--focus-ring-color);
  outline-offset: var(--focus-ring-offset);
}

button:disabled {
  background-color: var(--color-bg-mute);
  color: var(--color-text-mute);
  cursor: default;
}

:host([variant="primary"]) button {
  background-color: var(--color-bg-brand);
  color: var(--color-text-inverse);
}

:host([variant="primary"]) button:not(:disabled):is(:hover, :active) {
  background-color: var(--color-bg-brand-hover);
}

:host([variant="secondary"]) button {
  background-color: var(--color-bg-active);
  color: var(--color-fg-active);
}

:host([variant="secondary"]) button:not(:disabled):is(:hover, :active) {
  background-color: var(--color-bg-active);
  color: var(--color-fg-active);
}

:host([variant="outline"]) button {
  background-color: transparent;
  border-color: var(--color-border);
  color: var(--color-text-secondary);
}

:host([variant="outline"]) button:not(:disabled):is(:hover, :active) {
  background-color: var(--color-border);
  border-color: var(--color-border);
}

:host([variant="ghost"]) button {
  background-color: transparent;
  border-color: transparent;
  color: var(--color-text-secondary);
}

:host([variant="ghost"]) button:not(:disabled):is(:hover, :active) {
  background-color: var(--color-overlay);
}

:host([inverse]) button {
  color: var(--color-text-inverse);
}

:host([variant="outline"][inverse]) button {
  border-color: var(--color-text-inverse);
}

:host([variant="outline"][inverse]) button:not(:disabled):is(:hover, :active),
:host([variant="ghost"][inverse]) button:not(:disabled):is(:hover, :active) {
  background-color: var(--color-overlay);
  border-color: var(--color-text-inverse);
}

:host([rounded]) button {
  border-radius: var(--radius-full);
}

::slotted(svg) {
  width: 24px;
  height: 24px;
  flex-shrink: 0;
  fill: currentColor;
}
`,Z="ui-icon-button",ht=document.createElement("template");ht.innerHTML=`
  <style>${te}</style>
  <button type="button" part="button">
    <slot></slot>
  </button>
`;function st(e){return e==="submit"||e==="reset"?e:"button"}class ee extends HTMLElement{static get observedAttributes(){return["disabled","aria-label","type"]}constructor(){super();const t=this.attachShadow({mode:"open",delegatesFocus:!0});t.append(ht.content.cloneNode(!0)),this.button=t.querySelector("button")}connectedCallback(){this.dataset.component=Z,this.syncAttributes()}attributeChangedCallback(){this.syncAttributes()}get disabled(){return this.hasAttribute("disabled")}set disabled(t){this.toggleAttribute("disabled",t)}get type(){return st(this.getAttribute("type"))}set type(t){this.setAttribute("type",st(t))}focus(t){this.button.focus(t)}click(){this.button.click()}syncAttributes(){this.button.disabled=this.disabled,this.button.type=this.type,this.tabIndex=this.disabled?-1:0;const t=this.getAttribute("aria-label");t?this.button.setAttribute("aria-label",t):this.button.removeAttribute("aria-label")}}function ne(){customElements.get(Z)||customElements.define(Z,ee)}const ie='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="m644-428-58-58q9-47-27-88t-93-32l-58-58q17-8 34.5-12t37.5-4q75 0 127.5 52.5T660-500q0 20-4 37.5T644-428Zm128 126-58-56q38-29 67.5-63.5T832-500q-50-101-143.5-160.5T480-720q-29 0-57 4t-55 12l-62-62q41-17 84-25.5t90-8.5q151 0 269 83.5T920-500q-23 59-60.5 109.5T772-302Zm20 246L624-222q-35 11-70.5 16.5T480-200q-151 0-269-83.5T40-500q21-53 53-98.5t73-81.5L56-792l56-56 736 736-56 56ZM222-624q-29 26-53 57t-41 67q50 101 143.5 160.5T480-280q20 0 39-2.5t39-5.5l-36-38q-11 3-21 4.5t-21 1.5q-75 0-127.5-52.5T300-500q0-11 1.5-21t4.5-21l-84-82Zm319 93Zm-151 75Z"/></svg>',oe='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="M480-320q75 0 127.5-52.5T660-500q0-75-52.5-127.5T480-680q-75 0-127.5 52.5T300-500q0 75 52.5 127.5T480-320Zm0-72q-45 0-76.5-31.5T372-500q0-45 31.5-76.5T480-608q45 0 76.5 31.5T588-500q0 45-31.5 76.5T480-392Zm0 192q-146 0-266-81.5T40-500q54-137 174-218.5T480-800q146 0 266 81.5T920-500q-54 137-174 218.5T480-200Zm0-300Zm0 220q113 0 207.5-59.5T832-500q-50-101-144.5-160.5T480-720q-113 0-207.5 59.5T128-500q50 101 144.5 160.5T480-280Z"/></svg>',re=`:host {
  display: block;
  width: 100%;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

input,
button,
select,
textarea {
  margin: 0;
  font: inherit;
  color: inherit;
}

input,
textarea {
  -webkit-user-select: auto;
}

p {
  margin: 0;
}

.form-control__input-wrapper input,
.password-toggle-btn {
  font: inherit;
}

.form-control {
  position: relative;
  display: flex;
  flex-direction: column;
  width: 100%;
}

.form-control label {
  font-size: var(--font-size-label);
  font-weight: var(--font-weight-regular);
  margin-bottom: var(--spacing-xs);
}

.form-helper-text {
  margin-top: var(--spacing-xs);
  font-size: var(--font-size-caption);
  line-height: 1.25;
  color: var(--color-text-secondary);
}

.form-control__input-wrapper {
  position: relative;
  display: flex;
  align-items: center;
  width: 100%;
  gap: var(--spacing-sm);
}

.form-control__input-wrapper input {
  position: relative;
  display: flex;
  align-items: center;
  width: 100%;
  border: var(--border-01) solid var(--color-border);
  min-height: 44px;
  padding: var(--spacing-xs) var(--spacing-sm);
  border-radius: var(--input-radius);
  background-color: var(--color-bg-input);
  color: var(--color-text);
  font-size: var(--font-size-label);
  line-height: normal;
  appearance: none;
  -webkit-appearance: none;
}

.form-control__input-wrapper input::placeholder {
  color: var(--color-text-mute);
}

.form-control__input-wrapper input[type="time"]::-webkit-calendar-picker-indicator,
.form-control__input-wrapper input[type="date"]::-webkit-calendar-picker-indicator {
  display: none;
  -webkit-appearance: none;
}

.form-control__input-wrapper input:focus-visible,
.password-toggle-btn:focus-visible {
  outline: var(--focus-ring-width) solid var(--focus-ring-color);
  outline-offset: var(--focus-ring-offset);
}

:host(.error) input,
:host([invalid]) input,
:host([aria-invalid="true"]) input,
.form-control__input-wrapper input.error {
  outline: var(--focus-ring-width) solid var(--color-meter-red);
  outline-offset: var(--focus-ring-offset);
}

.duration-fields {
  align-items: center;
}

.duration-fields input {
  min-width: 0;
  flex: 1 1 0;
}

.duration-fields__unit {
  flex: 0 0 auto;
  font-size: var(--font-size-caption);
  font-weight: var(--font-weight-regular);
  color: var(--color-text-secondary);
  text-transform: uppercase;
}

.duration-fields__unit[hidden] {
  display: none;
}

.password-toggle-btn {
  position: absolute;
  right: 6px;
  display: flex;
  width: 36px;
  height: 36px;
  align-items: center;
  justify-content: center;
  padding: 0;
  border: none;
  border-radius: var(--radius-xs);
  background-color: transparent;
  color: var(--color-text-secondary);
  cursor: pointer;
  appearance: none;
  -webkit-appearance: none;
}

.password-toggle-btn:disabled {
  color: var(--color-text-mute);
  cursor: default;
}

.password-toggle-btn:not(:disabled):is(:hover, :active) {
  background-color: var(--color-overlay);
}

.password-icon {
  width: 24px;
  height: 24px;
  fill: var(--color-text-secondary);
}
`,W="ui-input",gt=document.createElement("template");gt.innerHTML=`
  <style>${re}</style>
  <div class="form-control">
    <label></label>
    <div class="form-control__input-wrapper">
      <input />
      <span class="duration-fields__unit"></span>
    </div>
    <p class="form-helper-text"></p>
  </div>
`;function se(e){return e==="number"||e==="password"||e==="time"||e==="date"?e:"text"}function ae(e){return e==="password"||e==="number"||e==="time"||e==="date"?e:"text"}class le extends HTMLElement{constructor(){super(),this.isPasswordVisible=!1,this.customValidationMessage="";const t=this.attachShadow({mode:"open",delegatesFocus:!0});t.append(gt.content.cloneNode(!0)),this.input=t.querySelector("input"),this.helperText=t.querySelector(".form-helper-text"),this.label=t.querySelector("label"),this.suffix=t.querySelector(".duration-fields__unit"),this.toggleButton=document.createElement("button"),this.wrapper=t.querySelector(".form-control__input-wrapper"),this.toggleButton.className="password-toggle-btn",this.toggleButton.type="button",this.input.addEventListener("input",n=>{this.syncValidationState(),n.stopPropagation(),this.dispatchEvent(new Event("input",{bubbles:!0,composed:!0}))}),this.input.addEventListener("change",n=>{this.syncValidationState(),n.stopPropagation(),this.dispatchEvent(new Event("change",{bubbles:!0,composed:!0}))}),this.input.addEventListener("invalid",()=>{this.syncValidationState()}),this.toggleButton.addEventListener("click",()=>{this.togglePasswordVisibility()})}static get observedAttributes(){return["aria-label","aria-invalid","autocomplete","disabled","helper-text","invalid","inputmode","label","max","min","name","placeholder","readonly","required","step","suffix","type","value","variant"]}connectedCallback(){this.hasAttribute("variant")||this.setAttribute("variant","text"),this.dataset.component=W,this.syncAttributes()}attributeChangedCallback(){this.syncAttributes()}get disabled(){return this.hasAttribute("disabled")}set disabled(t){this.toggleAttribute("disabled",t)}get type(){return this.input.type}set type(t){this.setAttribute("type",t)}get value(){return this.input.value}set value(t){this.input.value=t,this.syncValidationState()}focus(t){this.input.focus(t)}select(){this.input.select()}get validationMessage(){return this.input.validationMessage}get validity(){return this.input.validity}get willValidate(){return this.input.willValidate}checkValidity(){const t=this.input.checkValidity();return this.syncValidationState(),t}reportValidity(){const t=this.input.reportValidity();return this.syncValidationState(),t}setCustomValidity(t){this.customValidationMessage=t,this.input.setCustomValidity(t),this.syncValidationState()}syncAttributes(){const t=se(this.getAttribute("variant")),n=this.getAttribute("helper-text")||"",i=this.id?`${this.id}Input`:"input",o=this.getAttribute("label")||"",a=this.getAttribute("suffix")||"",l=t==="password"?this.isPasswordVisible?"text":"password":this.getAttribute("type")||ae(t);if(this.input.id=i,this.input.type=l,this.input.disabled=this.disabled,this.tabIndex=this.disabled?-1:0,this.input.readOnly=this.hasAttribute("readonly"),this.input.required=this.hasAttribute("required"),this.syncStringAttribute("aria-label"),this.syncStringAttribute("autocomplete"),this.syncStringAttribute("inputmode"),this.syncStringAttribute("max"),this.syncStringAttribute("min"),this.syncStringAttribute("name"),this.syncStringAttribute("placeholder"),this.syncStringAttribute("step"),this.hasAttribute("value")&&this.input.value!==this.getAttribute("value")&&(this.input.value=this.getAttribute("value")||""),this.input.setCustomValidity(this.customValidationMessage),this.label.htmlFor=i,this.label.textContent=o,this.label.hidden=o.length===0,this.helperText.textContent=n,this.helperText.hidden=n.length===0,this.wrapper.classList.toggle("duration-fields",t==="number"),this.suffix.textContent=a,this.suffix.hidden=t!=="number"||a.length===0,n.length>0){const c=`${i}HelperText`;this.helperText.id=c,this.input.setAttribute("aria-describedby",c)}else this.helperText.removeAttribute("id"),this.input.removeAttribute("aria-describedby");t==="password"?(this.toggleButton.isConnected||this.wrapper.append(this.toggleButton),this.toggleButton.disabled=this.disabled,this.toggleButton.setAttribute("aria-label",this.isPasswordVisible?"Hide password":"Show password"),this.toggleButton.innerHTML=ce(this.isPasswordVisible?ie:oe,"password-icon")):(this.isPasswordVisible=!1,this.toggleButton.remove()),this.syncValidationState()}syncStringAttribute(t){const n=this.getAttribute(t);if(n===null){this.input.removeAttribute(t);return}this.input.setAttribute(t,n)}togglePasswordVisibility(){this.isPasswordVisible=!this.isPasswordVisible,this.syncAttributes()}syncValidationState(){const t=this.hasAttribute("invalid")||this.getAttribute("aria-invalid")==="true"||!this.input.validity.valid;this.classList.toggle("error",t),this.input.classList.toggle("error",t),this.hasAttribute("invalid")!==t&&this.toggleAttribute("invalid",t),this.getAttribute("aria-invalid")!==String(t)&&this.setAttribute("aria-invalid",String(t)),this.input.getAttribute("aria-invalid")!==String(t)&&this.input.setAttribute("aria-invalid",String(t))}}function ce(e,t){return e.includes("class=")?e.replace("<svg",`<svg class="${t}"`):e.replace("<svg",`<svg class="${t}"`)}function de(){customElements.get(W)||customElements.define(W,le)}const ue=`:host {
  display: block;
  width: 100%;
}

:host([hidden]) {
  display: none;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

ol,
ul,
menu,
summary {
  margin: 0;
  padding: 0;
  list-style: none;
}

p {
  margin: 0;
}

.networks {
  position: relative;
  width: 100%;
  display: flex;
  flex-direction: column;
  margin-bottom: var(--spacing-lg);
}

.networks__list {
  position: relative;
  display: flex;
  width: 100%;
  flex-direction: column;
  height: 320px;
  overflow-y: auto;
  border: var(--border-01) solid var(--color-border);
  padding: var(--spacing-xs);
  border-radius: var(--radius-lg);
}

.networks__list:focus {
  outline: var(--focus-ring-width) solid var(--focus-ring-color);
  outline-offset: var(--focus-ring-offset);
}

.networks__list-empty-state {
  position: relative;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  color: var(--color-text-secondary);
  font-size: var(--font-size-heading);
}

.wifi-find-icon {
  width: 64px;
  height: 64px;
  fill: var(--color-text-mute);
}

.networks__item {
  position: relative;
  width: 100%;
  display: flex;
  align-items: center;
  cursor: pointer;
  padding: var(--spacing-sm) var(--spacing-xs);
  border: var(--border-01) solid transparent;
  min-height: 32px;
  font-size: var(--font-size-label);
  justify-content: space-between;
  border-radius: var(--radius-xs);
}

.networks__item-ssid {
  font-weight: var(--font-weight-semibold);
}

.networks__item-details {
  display: flex;
  align-items: center;
  gap: var(--spacing-md);
}

.networks__item ~ .networks__item {
  border-top: var(--border-01) solid var(--color-bg-selected);
}

.networks__item[aria-selected="true"] {
  background-color: var(--color-bg-active);
  outline: var(--focus-ring-width) solid var(--color-fg-active);
  outline-offset: var(--focus-ring-offset);
}

.wifi-security-icon,
.wifi-signal-icon,
.wifi-connected-icon {
  width: 16px;
  height: 16px;
  fill: var(--color-text-secondary);
}
`,J="ui-network-list",ft="networkList",K="network-list-label",mt=document.createElement("template");mt.innerHTML=`
  <style>${ue}</style>
  <div class="networks">
    <span id="${K}" hidden>Available networks</span>
    <ul
      id="${ft}"
      class="networks__list"
      role="listbox"
      aria-labelledby="${K}"
      tabindex="0"
    ></ul>
  </div>
`;class he extends HTMLElement{static get observedAttributes(){return["aria-label","label"]}constructor(){super();const t=this.attachShadow({mode:"open",delegatesFocus:!0});t.append(mt.content.cloneNode(!0)),this.labelElement=t.querySelector(`#${K}`),this.listElement=t.querySelector(`#${ft}`),this.listElement.addEventListener("keydown",n=>{(n.key==="ArrowDown"||n.key==="ArrowUp"||n.key==="Home"||n.key==="End"||n.key===" ")&&n.preventDefault()})}connectedCallback(){this.tabIndex=0,this.dataset.component=J,this.syncAttributes()}attributeChangedCallback(){this.syncAttributes()}appendItem(t){this.listElement.appendChild(t)}clearItems(){this.listElement.innerHTML=""}focus(t){this.listElement.focus(t)}getOptions(){return Array.from(this.listElement.querySelectorAll('[role="option"]'))}scrollOptionIntoView(t){const n=this.getOptions()[t];if(!n)return;const i=this.listElement.scrollTop,o=i+this.listElement.clientHeight,a=n.offsetTop,l=a+n.offsetHeight;if(a<i){this.listElement.scrollTop=a;return}l>o&&(this.listElement.scrollTop=l-this.listElement.clientHeight)}setActiveDescendant(t){if(t){this.listElement.setAttribute("aria-activedescendant",t);return}this.listElement.removeAttribute("aria-activedescendant")}setEmptyState(t){this.clearItems(),this.appendItem(t)}syncAttributes(){const t=this.getAttribute("label")||"Available networks",n=this.getAttribute("aria-label");this.labelElement.textContent=t,n?this.listElement.setAttribute("aria-label",n):this.listElement.removeAttribute("aria-label")}}function ge(){customElements.get(J)||customElements.define(J,he)}const fe=`:host {
  display: block;
  width: 100%;
}

:host([hidden]) {
  display: none;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

.wifi-status-card {
  position: relative;
  display: flex;
  width: 100%;
  align-items: center;
  background-color: var(--color-bg-brand);
  padding: var(--spacing-md) var(--spacing-md);
  border-radius: var(--radius-lg);
}

.wifi-status-card__content {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: var(--spacing-sm);
  flex-wrap: wrap;
  width: 100%;
}

.wifi-status-card__content .wifi-icon {
  width: 32px;
  height: 32px;
  fill: var(--color-text-inverse);
}

.wifi-status-card__actions {
  position: relative;
  display: flex;
  align-items: center;
  gap: var(--spacing-sm);
  margin-left: auto;
}

.wifi-status-card__status {
  position: relative;
  display: flex;
  align-items: center;
  gap: var(--spacing-xs);
  min-width: 0;
}

.wifi-status-card__label {
  font-size: var(--font-size-label);
  color: var(--color-text-inverse);
  font-weight: var(--font-weight-medium);
}

.wifi-status-card__network {
  font-size: var(--font-size-label);
  color: var(--color-text-inverse);
  font-weight: var(--font-weight-semibold);
  min-width: 0;
  overflow-wrap: anywhere;
}
`,G="ui-network-status",pt="actionBtn",bt="settingsBtn",vt="wifiStatusLabel",wt="wifiStatusIcon",yt="wifiStatusNetwork",Ct=document.createElement("template");Ct.innerHTML=`
  <style>${fe}</style>
  <div class="wifi-status-card">
    <div class="wifi-status-card__content">
      <div class="wifi-status-card__status">
        <span class="card-icon" id="${wt}"></span>
        <span id="${vt}" class="wifi-status-card__label">
          Disconnected
        </span>
        <span
          id="${yt}"
          class="wifi-status-card__network"
        ></span>
      </div>
      <div class="wifi-status-card__actions">
        <ui-button
          id="${pt}"
          variant="outline"
          inverse
          aria-label="Connect"
        >
          Connect
        </ui-button>
        <ui-button
          id="${bt}"
          variant="outline"
          inverse
          aria-label="Settings"
        >
          Settings
        </ui-button>
      </div>
    </div>
  </div>
`;function me(e,t){return e.includes("class=")?e.replace("<svg",`<svg class="${t}"`):e.replace("<svg",`<svg class="${t}"`)}class pe extends HTMLElement{static get observedAttributes(){return["action-label","network","status-label"]}constructor(){super();const t=this.attachShadow({mode:"open"});t.append(Ct.content.cloneNode(!0)),this.actionButton=t.querySelector(`#${pt}`),this.actionButton.addEventListener("click",n=>{n.stopPropagation(),this.dispatchEvent(new Event("action",{bubbles:!0,composed:!0}))}),this.iconElement=t.querySelector(`#${wt}`),this.labelElement=t.querySelector(`#${vt}`),this.networkElement=t.querySelector(`#${yt}`),this.settingsButton=t.querySelector(`#${bt}`),this.settingsButton.addEventListener("click",n=>{n.stopPropagation(),this.dispatchEvent(new Event("settings",{bubbles:!0,composed:!0}))})}connectedCallback(){this.dataset.component=G,this.syncAttributes()}attributeChangedCallback(){this.syncAttributes()}get actionDisabled(){return!!this.actionButton.disabled}set actionDisabled(t){this.actionButton.disabled=t}get iconSvg(){return this.iconElement.innerHTML}set iconSvg(t){this.iconElement.innerHTML=t?me(t,"wifi-icon"):""}get network(){return this.getAttribute("network")||""}set network(t){this.setAttribute("network",t)}get actionLabel(){return this.getAttribute("action-label")||"Connect"}set actionLabel(t){this.setAttribute("action-label",t)}get statusLabel(){return this.getAttribute("status-label")||"Disconnected"}set statusLabel(t){this.setAttribute("status-label",t)}syncAttributes(){this.actionButton.textContent=this.actionLabel,this.actionButton.setAttribute("aria-label",this.actionLabel),this.labelElement.textContent=this.statusLabel,this.networkElement.textContent=this.network,this.networkElement.hidden=this.network.length===0}}function be(){customElements.get(G)||customElements.define(G,pe)}const ve=`:host {
  display: block;
  width: 100%;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

input,
textarea {
  margin: 0;
  font: inherit;
  color: inherit;
  -webkit-user-select: auto;
}

.form-control {
  position: relative;
  display: flex;
  flex-direction: column;
  width: 100%;
}

.form-control label {
  font-size: var(--font-size-label);
  font-weight: var(--font-weight-regular);
  margin-bottom: var(--spacing-xs);
}

.range-slider {
  width: 100%;
  display: flex;
  align-items: center;
  gap: var(--spacing-sm);
}

.range-slider__range {
  -webkit-appearance: none;
  appearance: none;
  width: 100%;
  height: 34px;
  border-radius: var(--radius-full);
  background: var(--color-bg-input);
  outline: none;
}

.range-slider__range:focus-visible {
  outline: var(--focus-ring-width) solid var(--focus-ring-color);
  outline-offset: var(--focus-ring-offset);
}

.range-slider__range::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 30px;
  height: 30px;
  border-radius: 50%;
  background: var(--color-bg);
  border: var(--border-03) solid var(--color-bg-brand);
  box-shadow: 0px 2px 6px -2px rgba(32, 32, 32, 0.4);
  cursor: pointer;
  transition: background 0.15s ease-in-out;
}

.range-slider__range::-webkit-slider-thumb:hover {
  background: var(--color-bg);
}

.range-slider__range:active::-webkit-slider-thumb {
  background: var(--color-bg);
}

.range-slider__range::-moz-range-thumb {
  width: 30px;
  height: 30px;
  border: var(--border-03) solid var(--color-bg-brand);
  border-radius: 50%;
  background: var(--color-bg);
  box-shadow: 0px 2px 6px -2px rgba(32, 32, 32, 0.4);
  cursor: pointer;
  transition: background 0.15s ease-in-out;
}

.range-slider__range::-moz-range-thumb:hover {
  background: var(--color-bg);
}

.range-slider__range:active::-moz-range-thumb {
  background: var(--color-bg);
}

.range-slider__value {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 48px;
  height: 28px;
  font-size: var(--font-size-label);
  font-weight: var(--font-weight-semibold);
  color: var(--color-text-inverse);
  background: var(--color-bg-brand);
  border-radius: var(--radius-md);
}
`,we=document.createElement("template");we.innerHTML=`
  <style>${ve}</style>
  <div class="form-control">
    <label></label>
    <div class="range-slider">
      <input class="range-slider__range" type="range" />
      <span class="range-slider__value"></span>
    </div>
  </div>
`;const ye='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="M480-80 240-320l57-57 183 183 183-183 57 57L480-80ZM298-584l-58-56 240-240 240 240-58 56-182-182-182 182Z"/></svg>',Ce=`:host {
  display: block;
  width: 100%;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

input,
button,
select,
textarea {
  margin: 0;
  font: inherit;
  color: inherit;
}

input,
textarea {
  -webkit-user-select: auto;
}

p {
  margin: 0;
}

.form-control {
  position: relative;
  display: flex;
  flex-direction: column;
  width: 100%;
}

.form-control label {
  font-size: var(--font-size-label);
  font-weight: var(--font-weight-regular);
  margin-bottom: var(--spacing-xs);
}

.form-helper-text {
  margin-top: var(--spacing-xs);
  font-size: var(--font-size-caption);
  line-height: 1.25;
  color: var(--color-text-secondary);
}

.form-control__input-wrapper {
  position: relative;
  display: flex;
  align-items: center;
  width: 100%;
  gap: var(--spacing-sm);
}

.form-control__input-wrapper select {
  position: relative;
  display: flex;
  align-items: center;
  width: 100%;
  border: var(--border-01) solid var(--color-border);
  min-height: 44px;
  padding: var(--spacing-xs) var(--spacing-sm);
  border-radius: var(--input-radius);
  background-color: var(--color-bg-input);
  color: var(--color-text);
  font-size: var(--font-size-label);
  line-height: normal;
  appearance: none;
  -webkit-appearance: none;
}

.select-icon {
  position: absolute;
  width: 32px;
  height: 18px;
  background-color: transparent;
  border-radius: var(--radius-xs);
  right: 2px;
  fill: var(--color-text-secondary);
  pointer-events: none;
}

.form-control__input-wrapper select:focus-visible {
  outline: var(--focus-ring-width) solid var(--focus-ring-color);
  outline-offset: var(--focus-ring-offset);
}

:host(.error) select,
:host([invalid]) select,
:host([aria-invalid="true"]) select,
.form-control__input-wrapper select.error {
  outline: var(--focus-ring-width) solid var(--color-meter-red);
  outline-offset: var(--focus-ring-offset);
}
`,j="ui-select",xt=document.createElement("template");xt.innerHTML=`
  <style>${Ce}</style>
  <div class="form-control">
    <label></label>
    <div class="form-control__input-wrapper">
      <select></select>
      <span class="select-icon"></span>
    </div>
    <p class="form-helper-text"></p>
  </div>
`;class xe extends HTMLElement{constructor(){super(),this.customValidationMessage="";const t=this.attachShadow({mode:"open",delegatesFocus:!0});t.append(xt.content.cloneNode(!0)),this.helperText=t.querySelector(".form-helper-text"),this.labelElement=t.querySelector("label"),this.select=t.querySelector("select");const n=t.querySelector(".select-icon");n.innerHTML=Te(ye,"select-icon"),this.select.addEventListener("change",i=>{this.syncValidationState(),i.stopPropagation(),this.dispatchEvent(new Event("change",{bubbles:!0,composed:!0}))}),this.select.addEventListener("input",i=>{this.syncValidationState(),i.stopPropagation(),this.dispatchEvent(new Event("input",{bubbles:!0,composed:!0}))}),this.select.addEventListener("invalid",()=>{this.syncValidationState()})}static get observedAttributes(){return["aria-invalid","aria-label","disabled","helper-text","invalid","label","name","required","value"]}connectedCallback(){this.moveLightDomOptions(),this.dataset.component=j,this.syncAttributes()}attributeChangedCallback(){this.syncAttributes()}get disabled(){return this.hasAttribute("disabled")}set disabled(t){this.toggleAttribute("disabled",t)}get innerHTML(){return this.select.innerHTML}set innerHTML(t){this.select.innerHTML=t,this.syncValidationState()}get value(){return this.select.value}set value(t){this.select.value=t,this.syncValidationState()}appendChild(t){return t instanceof HTMLOptionElement||t instanceof HTMLOptGroupElement?(this.select.appendChild(t),t):super.appendChild(t)}focus(t){this.select.focus(t)}get validationMessage(){return this.select.validationMessage}get validity(){return this.select.validity}get willValidate(){return this.select.willValidate}checkValidity(){const t=this.select.checkValidity();return this.syncValidationState(),t}reportValidity(){const t=this.select.reportValidity();return this.syncValidationState(),t}setCustomValidity(t){this.customValidationMessage=t,this.select.setCustomValidity(t),this.syncValidationState()}moveLightDomOptions(){Array.from(this.children).forEach(t=>{(t instanceof HTMLOptionElement||t instanceof HTMLOptGroupElement)&&this.select.appendChild(t)})}syncAttributes(){const t=this.getAttribute("helper-text")||"",n=this.id?`${this.id}Select`:"select",i=this.getAttribute("label")||"";if(this.select.id=n,this.select.disabled=this.disabled,this.tabIndex=this.disabled?-1:0,this.select.required=this.hasAttribute("required"),this.syncStringAttribute("aria-label"),this.syncStringAttribute("name"),this.hasAttribute("value")&&(this.select.value=this.getAttribute("value")||""),this.select.setCustomValidity(this.customValidationMessage),this.labelElement.htmlFor=n,this.labelElement.textContent=i,this.labelElement.hidden=i.length===0,this.helperText.textContent=t,this.helperText.hidden=t.length===0,t.length>0){const o=`${n}HelperText`;this.helperText.id=o,this.select.setAttribute("aria-describedby",o)}else this.helperText.removeAttribute("id"),this.select.removeAttribute("aria-describedby");this.syncValidationState()}syncStringAttribute(t){const n=this.getAttribute(t);if(n===null){this.select.removeAttribute(t);return}this.select.setAttribute(t,n)}syncValidationState(){const t=this.hasAttribute("invalid")||this.getAttribute("aria-invalid")==="true"||!this.select.validity.valid;this.classList.toggle("error",t),this.select.classList.toggle("error",t),this.hasAttribute("invalid")!==t&&this.toggleAttribute("invalid",t),this.getAttribute("aria-invalid")!==String(t)&&this.setAttribute("aria-invalid",String(t)),this.select.getAttribute("aria-invalid")!==String(t)&&this.select.setAttribute("aria-invalid",String(t))}}function Se(){customElements.get(j)||customElements.define(j,xe)}function Te(e,t){return e.includes("class=")?e.replace("<svg",`<svg class="${t}"`):e.replace("<svg",`<svg class="${t}"`)}const Ae=`:host {
  display: block;
  width: 100%;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

.form-control {
  position: relative;
  display: flex;
  flex-direction: column;
  width: 100%;
}

.switch-container {
  display: flex;
  align-items: center;
  gap: var(--spacing-sm);
}

.switch-label {
  font-size: var(--font-size-label);
  font-weight: var(--font-weight-medium);
  flex-shrink: 0;
}

button {
  all: unset;
  box-sizing: border-box;
  position: relative;
  display: inline-flex;
  align-items: center;
  flex-shrink: 0;
  width: 62px;
  height: 34px;
  padding: 0;
  border: none;
  border-radius: var(--radius-full);
  background-color: var(--color-bg-input);
  color: inherit;
  cursor: pointer;
  transition: background-color 0.2s ease;
}

button:disabled {
  cursor: not-allowed;
  opacity: 0.6;
}

.switch-label[hidden] {
  display: none;
}

button[data-checked] {
  background-color: var(--color-bg-brand);
}

button::before {
  content: "";
  position: absolute;
  inset: 0;
  border-radius: inherit;
  opacity: 0;
  box-shadow: 0 0 0 var(--focus-ring-width) currentColor;
  transition: opacity 0.2s ease;
}

button:focus-visible::before {
  opacity: 0.35;
}

.knob {
  position: absolute;
  left: 2px;
  width: 30px;
  height: 30px;
  border-radius: var(--radius-full);
  background-color: var(--color-fg-knob-off);
  box-shadow: 0 2px 6px -2px rgba(32, 32, 32, 0.4);
  transition:
    transform 0.2s cubic-bezier(0.175, 0.885, 0.32, 1.275),
    background-color 0.2s ease;
}

button[data-checked] .knob {
  background-color: var(--color-fg-knob-on);
}
`,ke=document.createElement("template");ke.innerHTML=`
  <style>${Ae}</style>
  <div class="form-control">
    <div class="switch-container">
      <button type="button" part="button">
        <span class="knob"></span>
      </button>
      <span class="switch-label"></span>
    </div>
  </div>
`;const Ee=`:host {
  display: contents;
}

*,
*::before,
*::after {
  box-sizing: border-box;
}

.toast-dialog {
  position: fixed;
  left: 50%;
  bottom: var(--spacing-4xl);
  margin: 0;
  padding: 0;
  border: none;
  background: transparent;
  color: inherit;
  width: min(50vw, calc(100vw - 2rem));
  max-width: calc(100vw - 2rem);
  pointer-events: none;
  z-index: 30;
}

.toast-dialog[open] {
  display: block;
  animation: toast-dialog-fade 2.6s ease forwards;
}

.toast-dialog::backdrop {
  display: none;
}

.toast-dialog__message {
  margin: 0;
  box-sizing: border-box;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding: var(--spacing-sm) var(--spacing-lg);
  min-height: 44px;
  overflow: hidden;
  border: var(--border-01) solid var(--color-meter-green);
  border-radius: var(--radius-lg);
  background: var(--color-bg-active);
  color: var(--color-text);
  box-shadow: 0 0.5rem 1.25rem var(--color-overlay);
  font-size: var(--font-size-label);
  line-height: 1.25;
  text-align: center;
  text-overflow: ellipsis;
}

.toast-dialog[data-status='error'] .toast-dialog__message,
.toast-dialog[data-status='danger'] .toast-dialog__message {
  border-color: var(--color-text-danger);
  background: var(--color-bg-danger);
  color: var(--color-text-danger);
}

.toast-dialog[data-status='warning'] .toast-dialog__message {
  border-color: var(--color-meter-yellow);
  background: var(--color-bg-selected);
  color: var(--color-text-warning);
}

@keyframes toast-dialog-fade {
  0% {
    opacity: 0;
    transform: translate(-50%, 0.75rem);
  }

  12%,
  78% {
    opacity: 1;
    transform: translate(-50%, 0);
  }

  100% {
    opacity: 0;
    transform: translate(-50%, -0.25rem);
  }
}
`,Y="ui-toast",St=document.createElement("template");St.innerHTML=`
  <style>${Ee}</style>
  <dialog class="toast-dialog" aria-live="polite">
    <p class="toast-dialog__message"></p>
  </dialog>
`;function at(e){return e==="success"||e==="info"||e==="warning"||e==="error"||e==="danger"?e:null}class Le extends HTMLElement{static get observedAttributes(){return["aria-label","message","status"]}constructor(){super();const t=this.attachShadow({mode:"open"});t.append(St.content.cloneNode(!0)),this.dialog=t.querySelector("dialog"),this.messageElement=t.querySelector(".toast-dialog__message")}connectedCallback(){this.dataset.component=Y,this.syncAttributes()}attributeChangedCallback(){this.syncAttributes()}get message(){return this.messageElement.textContent||""}set message(t){this.setAttribute("message",t)}get status(){return at(this.getAttribute("status"))}set status(t){if(t){this.setAttribute("status",t);return}this.removeAttribute("status")}show(t,n){typeof t=="string"&&(this.message=t),n&&(this.status=n),this.dialog.open||this.dialog.show()}close(t){this.dialog.close(t)}syncAttributes(){const t=this.getAttribute("aria-label"),n=this.getAttribute("message")||"",i=at(this.getAttribute("status"));t?this.dialog.setAttribute("aria-label",t):this.dialog.removeAttribute("aria-label"),this.messageElement.textContent=n,this.messageElement.id=this.id?`${this.id}Message`:"statusToastMessage",this.dialog.id=this.id||"statusToast",i?this.dialog.setAttribute("data-status",i):this.dialog.removeAttribute("data-status")}}function _e(){customElements.get(Y)||customElements.define(Y,Le)}const Ie=`<svg width="260" height="93" viewBox="0 0 260 93" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M243.869 16.7734C246.778 16.7735 249.246 17.3268 251.272 18.4336C253.299 19.4666 254.961 21.1643 256.26 23.5254C257.559 25.8128 258.493 28.8385 259.064 32.6016C259.688 36.2908 260 40.7546 260 45.9932C260 51.2319 259.688 55.733 259.064 59.4961C258.493 63.1854 257.559 66.2111 256.26 68.5723C254.961 70.9334 253.272 72.631 251.194 73.6641C249.168 74.7708 246.726 75.3242 243.869 75.3242C240.908 75.3242 238.361 74.7709 236.231 73.6641C235.938 73.5216 235.653 73.3655 235.375 73.1992V90.708L221.659 92.2578V18.3232H228.438L231.045 23.6045C231.06 23.5782 231.074 23.5515 231.089 23.5254C232.388 21.1643 234.101 19.4666 236.231 18.4336C238.361 17.3268 240.908 16.7734 243.869 16.7734ZM77.5605 59.9199C79.3329 60.0681 80.9789 60.217 82.498 60.3652C84.0173 60.4393 85.4736 60.5128 86.8662 60.5869L85.1572 73.8164C81.7389 74.3352 77.9328 74.7426 76.6016 75.0391C75.2704 75.3354 73.9922 75.4844 72.7676 75.4844C69.6261 75.4843 67.2825 74.5949 65.7383 72.8164C64.3086 71.0399 63.5657 68.3427 63.5068 64.7266C64.0174 63.2715 64.435 61.7087 64.7617 60.0449L64.7637 60.0371C65.5282 56.0661 65.8984 51.4211 65.8984 46.1289C65.8984 41.1018 65.5728 36.7017 64.9004 32.9541L64.7617 32.2139C64.4345 30.5162 64.0166 28.9283 63.502 27.46V1.66797L77.5605 0V59.9199ZM103.92 59.9199C105.66 60.0654 107.278 60.211 108.773 60.3564C109.521 63.796 110.614 66.7901 112.103 69.2783L111.517 73.8164C108.098 74.3352 104.292 74.7426 102.961 75.0391C101.63 75.3354 100.352 75.4844 99.127 75.4844C95.9855 75.4843 93.6419 74.5949 92.0977 72.8164C90.6067 70.9636 89.8614 68.1095 89.8613 64.2559V1.66797L103.92 0V59.9199ZM43.7715 16.7734C47.3069 16.7734 50.3228 17.3295 52.8184 18.4414C55.3139 19.4792 57.3416 21.1845 58.9014 23.5566C60.5131 25.8546 61.6823 28.8943 62.4102 32.6748C63.138 36.3812 63.502 40.8659 63.502 46.1289C63.5019 51.3178 63.138 55.8025 62.4102 59.583C61.6823 63.2894 60.5131 66.3291 58.9014 68.7012C57.3417 70.9991 55.3138 72.7035 52.8184 73.8154C50.3228 74.9274 47.3069 75.4834 43.7715 75.4834C40.2363 75.4834 37.2211 74.9273 34.7256 73.8154C32.2301 72.7035 30.1762 70.9991 28.5645 68.7012C27.0047 66.3291 25.8346 63.2894 25.0547 59.583C24.3268 55.8025 23.9629 51.3178 23.9629 46.1289C23.9629 45.621 23.9678 45.1203 23.9746 44.627C23.9762 44.5097 23.9765 44.3928 23.9785 44.2764C23.9864 43.8165 23.9979 43.3631 24.0117 42.916C24.0153 42.8007 24.0194 42.6858 24.0234 42.5713C24.0396 42.1046 24.0592 41.645 24.082 41.1924C24.0901 41.0326 24.0985 40.8738 24.1074 40.7158C24.1128 40.6191 24.1183 40.5228 24.124 40.4268C24.1329 40.2782 24.1417 40.1304 24.1514 39.9834C24.328 37.2806 24.6287 34.8443 25.0547 32.6748C25.8346 28.8942 27.0047 25.8546 28.5645 23.5566C29.5167 22.1551 30.624 20.9875 31.8848 20.0518C31.9036 20.0377 31.9225 20.0237 31.9414 20.0098C32.0486 19.9311 32.1572 19.8546 32.2666 19.7793C32.2959 19.7591 32.325 19.7387 32.3545 19.7188C32.4538 19.6515 32.5542 19.586 32.6553 19.5215C32.6971 19.4948 32.7392 19.4686 32.7812 19.4424C32.8787 19.3818 32.9771 19.3228 33.0762 19.2646C33.1172 19.2406 33.1579 19.216 33.1992 19.1924C33.2949 19.1376 33.3911 19.0837 33.4883 19.0312C33.5403 19.0032 33.5931 18.9766 33.6455 18.9492C33.7419 18.8988 33.8387 18.8489 33.9365 18.8008C33.9803 18.7792 34.0243 18.7584 34.0684 18.7373C34.1661 18.6905 34.2641 18.6442 34.3633 18.5996C34.3848 18.5899 34.4062 18.5799 34.4277 18.5703L34.4268 18.5713C34.5257 18.5274 34.6252 18.4832 34.7256 18.4414C37.221 17.3296 40.2364 16.7735 43.7715 16.7734ZM129.773 16.7734C133.309 16.7734 136.325 17.3295 138.82 18.4414C141.316 19.4792 143.344 21.1845 144.903 23.5566C146.515 25.8546 147.685 28.8943 148.413 32.6748C148.784 34.5656 149.059 36.6592 149.241 38.9551L142.836 41.3262C137.822 43.1818 137.822 50.2732 142.836 52.1289L149.135 54.46C148.958 56.2912 148.718 57.999 148.413 59.583C147.685 63.2894 146.515 66.3291 144.903 68.7012C143.344 70.999 141.316 72.7035 138.82 73.8154C136.325 74.9273 133.309 75.4834 129.773 75.4834C126.238 75.4833 123.223 74.9273 120.728 73.8154C118.232 72.7035 116.178 70.9991 114.566 68.7012C113.846 67.6055 113.21 66.3668 112.656 64.9863V64.9883C112.643 64.955 112.63 64.9211 112.617 64.8877C112.567 64.7609 112.517 64.6331 112.468 64.5039C112.453 64.465 112.438 64.4259 112.424 64.3867C112.306 64.0717 112.192 63.7499 112.083 63.4209C112.042 63.2964 112.002 63.1705 111.962 63.0439C111.618 61.9632 111.316 60.8099 111.058 59.583C110.33 55.8025 109.966 51.3178 109.966 46.1289C109.966 40.8659 110.33 36.3812 111.058 32.6748C111.837 28.8943 113.007 25.8546 114.566 23.5566C116.178 21.1846 118.232 19.4792 120.728 18.4414C123.223 17.3296 126.238 16.7735 129.773 16.7734ZM194.655 61.9453H200.43C201.886 61.9453 202.9 61.6464 203.473 61.0498C204.045 60.453 204.331 59.4455 204.331 58.0283V17.9717H218.064V73.917H211.276L207.765 68.3223C205.58 70.7091 203.187 72.4999 200.586 73.6934C197.985 74.8868 195.201 75.4834 192.236 75.4834C188.335 75.4834 185.474 74.29 183.653 71.9033C181.833 69.4417 180.922 65.5627 180.922 60.2666V53.8076L185.459 52.1289C190.473 50.2731 190.473 43.1808 185.459 41.3252L180.922 39.6455V17.9717H194.655V61.9453ZM34.7266 15.8535C34.4142 15.9718 34.1065 16.0962 33.8047 16.2295V16.2285C31.4506 17.2075 29.4409 18.6911 27.7812 20.6367H13.9463V33.8584H22.4111C21.8428 37.4038 21.5664 41.5008 21.5664 46.1289C21.5664 46.746 21.5719 47.3546 21.582 47.9541H13.9463V73.0869H0V4.79199H35.9443L34.7266 15.8535ZM163.023 27C163.409 25.9569 164.885 25.9569 165.271 27L170.104 40.0625C170.226 40.3904 170.485 40.6491 170.812 40.7705L183.876 45.6045C183.995 45.6486 184.1 45.7071 184.192 45.7764C184.248 45.8181 184.297 45.8642 184.343 45.9131C184.364 45.9355 184.382 45.9597 184.4 45.9834C184.417 46.0043 184.434 46.0241 184.449 46.0459C184.465 46.0691 184.478 46.094 184.492 46.1182C184.509 46.1465 184.525 46.1746 184.539 46.2041C184.548 46.2237 184.556 46.2437 184.564 46.2637C184.577 46.2943 184.589 46.325 184.599 46.3564C184.607 46.3826 184.614 46.409 184.62 46.4355C184.627 46.4644 184.634 46.4932 184.639 46.5225C184.643 46.5487 184.646 46.5751 184.648 46.6016C184.651 46.6299 184.654 46.6581 184.655 46.6865C184.656 46.7136 184.656 46.7405 184.655 46.7676C184.654 46.8006 184.651 46.8334 184.647 46.8662C184.645 46.8881 184.642 46.9099 184.639 46.9316C184.633 46.9629 184.627 46.9936 184.619 47.0244C184.612 47.0514 184.605 47.078 184.597 47.1045C184.588 47.1306 184.579 47.1561 184.568 47.1816C184.557 47.2097 184.546 47.2374 184.532 47.2646C184.521 47.2881 184.507 47.3103 184.494 47.333C184.479 47.3585 184.466 47.3847 184.449 47.4092C184.432 47.435 184.411 47.4589 184.392 47.4834C184.374 47.5049 184.357 47.5265 184.338 47.5469C184.315 47.5708 184.29 47.592 184.266 47.6143C184.246 47.6322 184.227 47.6512 184.205 47.668C184.199 47.673 184.192 47.6777 184.186 47.6826C184.095 47.75 183.993 47.8084 183.876 47.8516L170.812 52.6855C170.485 52.807 170.226 53.0657 170.104 53.3936L165.271 66.4561C164.885 67.4992 163.409 67.4992 163.023 66.4561L158.19 53.3936C158.069 53.0657 157.81 52.8069 157.482 52.6855L144.419 47.8516C143.376 47.4655 143.376 45.9907 144.419 45.6045L144.762 45.4775L145.382 45.2471L149.477 43.7314V43.7324L157.482 40.7705C157.81 40.6491 158.069 40.3904 158.19 40.0625L163.023 27ZM238.491 31.6055C237.297 31.6055 236.466 31.8269 235.998 32.2695C235.583 32.6385 235.375 33.3759 235.375 34.4824V62.7734L242.076 60.4922C243.323 60.4922 244.155 60.3084 244.57 59.9395C245.038 59.4967 245.271 58.6845 245.271 57.5039V29.3242L238.491 31.6055ZM41.9004 31.6738C40.6526 31.6738 39.7941 31.896 39.3262 32.3408C38.9104 32.7115 38.7031 33.49 38.7031 34.6758V62.584L45.6436 60.584C46.8909 60.584 47.7226 60.3987 48.1387 60.0283C48.6065 59.5836 48.8408 58.7679 48.8408 57.582V29.6738L41.9004 31.6738ZM127.902 31.6738C126.655 31.6738 125.797 31.8962 125.329 32.3408C124.913 32.7115 124.705 33.4899 124.705 34.6758V62.584L131.646 60.584C132.893 60.584 133.726 60.3989 134.142 60.0283C134.609 59.5836 134.843 58.7678 134.843 57.582V29.6738L127.902 31.6738Z" fill="currentColor"/>
</svg>
`,Be='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="m109-531-85-85q92-89 210-136.5T480-800q128 0 246 47.5T936-616l-85 85q-75-72-171-110.5T480-680q-104 0-200 38.5T109-531Zm169 169-84-84q59-55 132.5-84.5T480-560q80 0 153.5 29.5T766-446l-84 84q-42-38-93.5-58T480-440q-57 0-108.5 20T278-362Zm202 202q-33 0-56.5-23.5T400-240q0-33 23.5-56.5T480-320q33 0 56.5 23.5T560-240q0 33-23.5 56.5T480-160Z"/></svg>',Me='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="M240-80q-33 0-56.5-23.5T160-160v-400q0-33 23.5-56.5T240-640h40v-80q0-83 58.5-141.5T480-920q83 0 141.5 58.5T680-720v80h40q33 0 56.5 23.5T800-560v400q0 33-23.5 56.5T720-80H240Zm0-80h480v-400H240v400Zm240-120q33 0 56.5-23.5T560-360q0-33-23.5-56.5T480-440q-33 0-56.5 23.5T400-360q0 33 23.5 56.5T480-280ZM360-640h240v-80q0-50-35-85t-85-35q-50 0-85 35t-35 85v80ZM240-160v-400 400Z"/></svg>',ze='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="M480-120 0-600q96-98 220-149t260-51q137 0 261 51t219 149L480-120ZM361-353q25-18 55.5-28t63.5-10q33 0 63.5 10t55.5 28l245-245q-78-59-170.5-90.5T480-720q-101 0-193.5 31.5T116-598l245 245Z"/></svg>',qe='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="M480-120 0-600q96-98 220-149t260-51q137 0 261 51t219 149L480-120ZM299-415q38-28 84-43.5t97-15.5q51 0 97 15.5t84 43.5l183-183q-78-59-170.5-90.5T480-720q-101 0-193.5 31.5T116-598l183 183Z"/></svg>',Ne='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="M480-120 0-600q96-98 220-149t260-51q137 0 261 51t219 149L480-120ZM232-482q53-38 116-59.5T480-563q69 0 132 21.5T728-482l116-116q-78-59-170.5-90.5T480-720q-101 0-193.5 31.5T116-598l116 116Z"/></svg>',Pe='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="M480-120 0-600q95-97 219.5-148.5T480-800q136 0 260.5 51.5T960-600L480-120Z"/></svg>',Ve='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="M480-120 0-601q93-93 215.5-146T480-800q142 0 264.5 53T960-601l-56 57q-81-81-190-128.5T480-720q-103 0-195 32.5T117-597l419 420-56 57Zm384-40L761-262q-18 11-38 16.5t-43 5.5q-68 0-114-46t-46-114q0-68 46-114t114-46q68 0 114 46t46 114q0 23-5.5 43T818-319l102 103-56 56ZM680-320q34 0 57-23t23-57q0-34-23-57t-57-23q-34 0-57 23t-23 57q0 34 23 57t57 23ZM480-177Z"/></svg>',Oe='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="M280-240q-100 0-170-70T40-480q0-100 70-170t170-70q66 0 121 33t87 87h432v240h-80v120H600v-120H488q-32 54-87 87t-121 33Zm0-80q66 0 106-40.5t48-79.5h246v120h80v-120h80v-80H434q-8-39-48-79.5T280-640q-66 0-113 47t-47 113q0 66 47 113t113 47Zm0-80q33 0 56.5-23.5T360-480q0-33-23.5-56.5T280-560q-33 0-56.5 23.5T200-480q0 33 23.5 56.5T280-400Zm0-80Z"/></svg>',De='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="M382-240 154-468l57-57 171 171 367-367 57 57-424 424Z"/></svg>',He='<svg xmlns="http://www.w3.org/2000/svg" height="24px" viewBox="0 -960 960 960" width="24px" fill="#e8eaed"><path d="m612-292 56-56-148-148v-184h-80v216l172 172ZM480-80q-83 0-156-31.5T197-197q-54-54-85.5-127T80-480q0-83 31.5-156T197-763q54-54 127-85.5T480-880q83 0 156 31.5T763-763q54 54 85.5 127T880-480q0 83-31.5 156T763-197q-54 54-127 85.5T480-80Zm0-400Zm0 320q133 0 226.5-93.5T800-480q0-133-93.5-226.5T480-800q-133 0-226.5 93.5T160-480q0 133 93.5 226.5T480-160Z"/></svg>',$e={"Content-Type":"application/json",Accept:"application/json"},Re=2e3,Fe=8,Ue=2e3,Ze=8,We=3e3,Je=750,Ke=20,Ge="/api/settings",je=`${Ge}/time`,Ye="/api/runtime/time";async function M(e,t){const n=await fetch(e,{cache:"no-store",...t,headers:{...$e,...t?.headers||{}}}),i=n.headers.get("content-type")||"",o=await n.text();if(!i.includes("application/json"))throw new Error(`Unexpected response for ${e}. Make sure you are connected to the device AP and loading the portal from the ESP32.`);const a=JSON.parse(o);if(!n.ok||a.success!==!0)throw new Error(a.message||"Request failed.");return a}const Xe=(e,t)=>M(e,t),Qe=(e,t)=>M(e,t),tn=(e,t)=>M(e,t),Tt=(e,t)=>M(e,t),en=(e,t)=>M(e,t);function p(e){const t=document.getElementById(e);if(!t)throw new Error(`Required portal element "#${e}" was not found.`);return t}function nn(){return{followupLogoEl:p("followupLogo"),wifiStatusCard:p("wifiStatusCard"),wifiSettingsSheet:p("wifiSettingsSheet"),wifiSettingsCloseBtn:p("wifiSettingsCloseBtn"),wifiSettingsNotification:p("wifiSettingsNotification"),networkList:p("networkList"),passwordInput:p("password"),scanBtn:p("scanBtn"),connectBtn:p("connectBtn"),localAiCard:p("localAiCard"),localAiBaseUrlInput:p("localAiBaseUrlInput"),localAiResetBtn:p("localAiResetBtn"),localAiSaveBtn:p("localAiSaveBtn"),timezoneLocationCard:p("timezoneLocationCard"),timezoneSelect:p("timezoneSelect"),manualTimeInput:p("manualTimeInput"),manualDateInput:p("manualDateInput"),timezoneLocationClearBtn:p("timezoneLocationClearBtn"),timezoneLocationSaveBtn:p("timezoneLocationSaveBtn")}}function At(e){return(t,n="info")=>{e.setNotification(t,n)}}function on(e){return(t,n="info")=>{if(!t){e.hidden=!0,e.textContent="",e.removeAttribute("data-status"),e.setAttribute("role","status"),e.setAttribute("aria-live","polite");return}e.hidden=!1,e.textContent=t,e.setAttribute("data-status",n),n==="error"||n==="danger"?(e.setAttribute("role","alert"),e.setAttribute("aria-live","assertive")):n==="warning"?(e.setAttribute("role","alert"),e.setAttribute("aria-live","polite")):(e.setAttribute("role","status"),e.setAttribute("aria-live","polite"))}}function Q(e){e.classList.remove("error"),e.removeAttribute("invalid"),e.setAttribute("aria-invalid","false"),e.setCustomValidity?.("")}function tt(e,t){e.classList.add("error"),e.setAttribute("invalid",""),e.setAttribute("aria-invalid","true"),e.setCustomValidity?.(t)}function rn(e,t){e.setAttribute("aria-checked",t?"true":"false")}function sn(e){return e.getAttribute("aria-checked")==="true"}function kt(e,t){return e.includes("class=")?e.replace("<svg",`<svg class="${t}"`):e.replace("<svg",`<svg class="${t}"`)}function an(e,t,n){e.innerHTML=kt(t,n)}function ln(e){return new Promise(t=>{setTimeout(t,e)})}function cn(){const e=document.documentElement.lang?.trim();return e||"en-US"}function dn(e){try{return new Intl.DateTimeFormat(cn(),{hour:"numeric",timeZone:e,timeZoneName:"longGeneric"}).formatToParts(new Date).find(i=>i.type==="timeZoneName")?.value?.trim()||""}catch(t){return console.warn(`Failed to format timezone label for "${e}".`,t),""}}function lt(e){return/^(GMT|UTC)(?:[+-]\d{1,2}(?::\d{2})?)?$/.test(e)}function un(e){return(e.split("/").pop()||e).replace(/_/g," ")}function Et(e){return e.split("/").map(t=>t.replace(/_/g," ")).join(" / ")}function hn(e){const t=e.map(o=>{const a=dn(o.name);return{cityLabel:un(o.name),genericLabel:a,name:o.name}}),n=new Map(t.map(o=>[o.name,o])),i=new Map;return t.forEach(o=>{!o.genericLabel||lt(o.genericLabel)||i.set(o.genericLabel,(i.get(o.genericLabel)||0)+1)}),new Map(e.map(o=>{const a=n.get(o.name),l=o.description?.trim()||"";if(a?.genericLabel&&!lt(a.genericLabel)){const u=(i.get(a.genericLabel)||0)>1?`${a.genericLabel} (${a.cityLabel})`:a.genericLabel;return[o.name,u]}return l?[o.name,l]:[o.name,Et(o.name)]}))}function gn(e,t){return t.get(e.name)||e.description?.trim()||Et(e.name)}function fn(e){const t=e.value.trim();if(!t)return null;const n=/^(\d{2}):(\d{2})$/.exec(t);if(!n)return null;const i=Number.parseInt(n[1],10),o=Number.parseInt(n[2],10);return!Number.isFinite(i)||!Number.isFinite(o)||i<0||i>23||o<0||o>59?null:i*60+o}function mn(e){const t={isBusy:!1,baseUrl:"",resetApi:"/api/settings/local_ai/reset",settingsApi:"/api/settings/local_ai"};function n(l){const c=l?.routes;t.settingsApi=c?.settings||"/api/settings/local_ai",t.resetApi=c?.reset||"/api/settings/local_ai/reset"}function i(l){t.baseUrl=typeof l?.base_url=="string"?l.base_url:"",e.localAiBaseUrlInput.value=t.baseUrl}async function o(){if(!e.isLocalAiModuleActive()||t.isBusy)return;const l=e.localAiBaseUrlInput.value.trim();if(!l){e.notifyLocalAi("Local AI server URL is required.","warning");return}t.isBusy=!0,e.notifyLocalAi("Saving local AI server URL...","info"),e.updateButtons();try{const c=await e.fetchLocalAiModuleJson(t.settingsApi,{method:"PATCH",body:JSON.stringify({base_url:l})});i(c.settings),e.notifyLocalAi(c.message||"Local AI server URL stored.","success")}catch(c){console.error("Local AI server URL save failed:",c),e.notifyLocalAi(c instanceof Error?c.message:"Failed to store local AI server URL.","error")}finally{t.isBusy=!1,e.updateButtons()}}async function a(){if(!(!e.isLocalAiModuleActive()||t.isBusy)){t.isBusy=!0,e.notifyLocalAi("Resetting local AI server URL...","info"),e.updateButtons();try{const l=await e.fetchLocalAiModuleJson(t.resetApi,{method:"POST"});i(l.settings),e.notifyLocalAi(l.message||"Local AI server URL reset to built-in default.","success")}catch(l){console.error("Local AI server URL reset failed:",l),e.notifyLocalAi(l instanceof Error?l.message:"Failed to reset local AI server URL.","error")}finally{t.isBusy=!1,e.updateButtons()}}}return{applyLocalAiSettings:i,getLocalAiBaseUrl:()=>t.baseUrl,isLocalAiBusy:()=>t.isBusy,resetLocalAiBaseUrl:a,saveLocalAiBaseUrl:o,updateLocalAiRoutes:n}}function pn(e){let t=!1,n=!1,i=!1,o=!1;async function a(){return e.fetchTimezoneListJson("/api/timezone/list")}function l(s){const h=s.settings,m=s.runtime;e.setSwitchChecked(e.clockModeToggle,!!h?.enabled),e.timezoneSelect.value=h?.timezone_name||"",e.clearFieldError(e.timezoneSelect),e.manualDateInput.value=typeof m?.current_date=="string"?m.current_date:"",e.manualTimeInput.value=typeof m?.current_time=="string"?m.current_time:""}function c(s){s&&(typeof s.clock_enabled=="boolean"&&e.setSwitchChecked(e.clockModeToggle,s.clock_enabled),e.manualDateInput.value=typeof s.current_date=="string"?s.current_date:"",e.manualTimeInput.value=typeof s.current_time=="string"?s.current_time:"")}async function u(){e.timezoneSelect.innerHTML="";const s=document.createElement("option");s.value="",s.textContent="Select timezone",e.timezoneSelect.appendChild(s);try{const h=await a(),m=Array.isArray(h.timezones)?h.timezones:[],w=e.buildTimezoneLabelMap(m);m.map(v=>({value:v.name,label:e.formatTimezoneLabel(v,w)})).sort((v,T)=>v.label.localeCompare(T.label)).forEach(v=>{const T=document.createElement("option");T.value=v.value,T.textContent=v.label,e.timezoneSelect.appendChild(T)})}catch(h){console.error("Timezone list fetch failed:",h),e.notify(h instanceof Error?h.message:"Failed to load timezones.","error")}}async function f(){if(!(t||n||i)){t=!0,n=!0,i=!0,e.onStateChange();try{const s=await e.fetchTimeSettingsJson(e.timeSettingsApi);l(s)}catch(s){console.error("Time settings status failed:",s),e.notify(s instanceof Error?s.message:"Time settings status failed.","error")}finally{t=!1,n=!1,i=!1,e.onStateChange()}}}async function y(){if(!(i||o)){o=!0;try{for(let s=0;s<e.clockSyncPollAttempts;s++){const h=await e.fetchTimeRuntimeJson(e.timeRuntimeApi);if(c(h.runtime),h.runtime?.time_valid===!0&&typeof h.runtime.current_time=="string"&&h.runtime.current_time.length>0)return;s<e.clockSyncPollAttempts-1&&await new Promise(m=>{setTimeout(m,e.clockSyncPollIntervalMs)})}}catch(s){console.error("Clock refresh after WiFi connect failed:",s)}finally{o=!1}}}async function E(){if(!(document.hidden||t||n||i)&&e.isSwitchChecked(e.clockModeToggle))try{const s=await e.fetchTimeRuntimeJson(e.timeRuntimeApi);c(s.runtime)}catch(s){console.error("Time runtime status failed:",s)}}function S(){const s=e.timezoneSelect.value.trim(),h=e.manualDateInput.value.trim(),m=e.manualTimeInput.value.trim(),w=e.parseClockTimeInputValue(e.wakeupTimeInput),v=e.parseClockTimeInputValue(e.bedtimeTimeInput);return!!h!=!!m?(e.notify("Provide both current date and time for a manual clock set.","error"),null):{timezoneName:s,manualDate:h,manualTime:m,wakeupMinutes:w??void 0,bedtimeMinutes:v??void 0}}async function A(s,h){return e.fetchTimeSettingsJson(e.timeSettingsApi,{method:"PATCH",body:JSON.stringify({timezone_name:s.timezoneName,enabled:h,manual_date:s.manualDate,manual_time:s.manualTime})})}async function C(){if(i||t||n){e.notifyClockMode("Clock mode update already in progress.","warning");return}const s=e.isSwitchChecked(e.clockModeToggle),h=!s,m=e.timezoneSelect.value.trim();e.setSwitchChecked(e.clockModeToggle,h),i=!0,e.notifyClockMode(h?"Enabling clock mode...":"Disabling clock mode...","info"),e.onStateChange();try{const w=await e.fetchTimeSettingsJson(e.timeSettingsApi,{method:"PATCH",body:JSON.stringify({enabled:h,timezone_name:h&&m?m:void 0})});l(w),e.notifyClockMode(w.message||(h?"Clock mode enabled.":"Clock mode disabled."),"success")}catch(w){e.setSwitchChecked(e.clockModeToggle,s),console.error("Clock mode toggle failed:",w);const v=w instanceof Error?w.message:"Failed to update clock mode.",T=v==="timezone_name required to enable clock"?"Please set timezone first.":v;v==="timezone_name required to enable clock"&&(e.setFieldError(e.timezoneSelect,"Select a timezone."),e.timezoneSelect.focus({preventScroll:!0})),e.notifyClockMode(T,"error")}finally{i=!1,e.onStateChange()}}async function x(){if(t||n||i||e.isTalkingClockModuleBusy()){e.notify("Time configuration update already in progress.","warning");return}const s=S();if(s){if(!s.timezoneName){e.setFieldError(e.timezoneSelect,"Select a timezone."),e.notify("Select a timezone.","error"),e.timezoneSelect.focus({preventScroll:!0});return}if(e.clearFieldError(e.timezoneSelect),e.isTalkingClockModuleActive()&&(s.wakeupMinutes===void 0||s.bedtimeMinutes===void 0)){e.notify("Wakeup and bedtime times are required for the talking clock module.","error"),e.focusTalkingClockTimeInput();return}t=!0,n=!0,i=!0,e.onTalkingClockBusyChange(e.isTalkingClockModuleActive()),e.notify("Saving time configuration...","info"),e.onStateChange();try{const h=await A(s,e.isSwitchChecked(e.clockModeToggle));if(l(h),e.isTalkingClockModuleActive()){const m=await e.patchTalkingClockSettings({wakeup_minutes:s.wakeupMinutes,bedtime_minutes:s.bedtimeMinutes});e.applyTalkingClockModuleSettings(m.settings)}e.notify("Time configuration saved successfully.","success")}catch(h){console.error("Timezone/location save failed:",h),e.notify(h instanceof Error?h.message:"Failed to save time configuration.","error")}finally{t=!1,n=!1,i=!1,e.onTalkingClockBusyChange(!1),e.onStateChange()}}}async function k(){if(t||n||i){e.notify("Time configuration update already in progress.","warning");return}t=!0,n=!0,i=!0,e.notify("Clearing timezone/location...","info"),e.onStateChange();try{const s=await e.fetchTimeSettingsJson(e.timeSettingsApi,{method:"PATCH",body:JSON.stringify({enabled:!1,timezone_name:""})});l(s),e.notify("Timezone/location cleared successfully.","success")}catch(s){console.error("Timezone/location clear failed:",s),e.notify(s instanceof Error?s.message:"Failed to clear timezone/location.","error")}finally{t=!1,n=!1,i=!1,e.onStateChange()}}return{applyTimeRuntimeStatus:c,applyTimeSettingsStatus:l,clearTimezoneLocation:k,fetchTimeRuntimeStatus:E,fetchTimeSettingsStatus:f,isClockBusy:()=>i,isLocationBusy:()=>n,isTimezoneBusy:()=>t,populateTimezoneOptions:u,refreshClockStatusAfterWifiConnect:y,saveTimezoneLocation:x,toggleClockModeSetting:C}}function bn(e,t){return e>=-50?t.fourBar:e>=-60?t.threeBar:e>=-70?t.twoBar:t.oneBar}function vn(e){let t=[],n="",i=!1,o="",a=!1,l=!1,c=!1,u=-1;function f(){e.wifiStatusCard.hidden=!1,e.wifiStatusCard.statusLabel=i?"Connected":"Disconnected",e.wifiStatusCard.network=i?o:""}function y(){const r=document.createElement("li");r.className="networks__list-empty-state",r.innerHTML=`${e.svgWithClass(e.wifiFindIcon,"wifi-find-icon")}<p>Scan for available networks</p>`,e.networkList.setEmptyState(r)}function E(){if(t.length===0){n="",u=-1;return}const r=t.findIndex(d=>d.ssid===n);if(r>=0){u=r;return}n="",u=-1}function S(){const r=e.networkList.getOptions();r.forEach((d,g)=>{d.setAttribute("aria-selected",String(n===t[g]?.ssid))}),u>=0&&u<r.length?e.networkList.setActiveDescendant(r[u].id):e.networkList.setActiveDescendant(null)}function A(){const r=t.find(d=>d.ssid===n);return r?!r.is_open:!0}function C(r){n=r,u=t.findIndex(d=>d.ssid===r),A()||e.clearFieldError(e.passwordInput),S(),e.onStateChange()}function x(){if(E(),e.networkList.clearItems(),t.length===0){y();return}t.forEach((r,d)=>{const g=document.createElement("li");g.className="networks__item",g.id=`network-option-${d}`,g.setAttribute("role","option"),g.setAttribute("tabindex","-1"),g.setAttribute("aria-selected",String(n===r.ssid));const N=document.createElement("span");N.className="networks__item-ssid",N.textContent=r.ssid;const I=document.createElement("div");if(I.className="networks__item-details",o===r.ssid&&i){const O=document.createElement("span");O.title="Currently connected",O.innerHTML=e.svgWithClass(e.checkIcon,"wifi-connected-icon"),I.appendChild(O)}const P=document.createElement("span");P.title=r.is_open?"Open":"Secured",r.is_open||(P.innerHTML=e.svgWithClass(e.securityIcon,"wifi-security-icon")),I.appendChild(P);const V=document.createElement("span");V.title=`Signal strength: ${r.signal_strength} (${r.rssi} dBm)`,V.innerHTML=e.svgWithClass(bn(r.rssi,e.signalIcons),"wifi-signal-icon"),I.appendChild(V),g.append(N,I),g.addEventListener("click",()=>{u=d,C(r.ssid),e.networkList.focus({preventScroll:!0})}),e.networkList.appendItem(g)}),S()}function k(r){const d=i,g=o;i=r.connected===!0,o=typeof r.ssid=="string"?r.ssid:"",f(),(d!==i||g!==o)&&x(),e.onConnectionStateChange?.({wasConnected:d,previousNetwork:g,isCurrentlyConnected:i,connectedNetwork:o})}async function s(){if(!a){a=!0,e.notify("Scanning for networks...","info"),e.onStateChange();try{let r="Scanning for networks...";for(let d=0;d<e.scanPollAttempts;d++){const g=await e.fetchPortalJson("/api/scan");if(r=g.message||r,g.scan_in_progress===!0){if(e.notify(r,"info"),d<e.scanPollAttempts-1){await e.delayMs(e.scanPollIntervalMs);continue}}else{t=Array.isArray(g.networks)?g.networks:[],e.notify(r||"Scan complete.","success"),x();return}}throw new Error("Network scan timed out. Please try again.")}catch(r){console.error("Scan failed:",r),e.notify(r instanceof Error?r.message:"Scan failed. Please try again.","error"),t=[],x()}finally{a=!1,e.onStateChange()}}}async function h(){if(!l){l=!0,e.onStateChange();try{const r=await e.fetchPortalJson("/api/status");k(r),a||e.notify(r.message||"Status updated.",r.connected?"success":"info")}catch(r){console.error("Status check failed:",r),e.notify(r instanceof Error?r.message:"Status check failed.","error")}finally{l=!1,e.onStateChange()}}}async function m(){if(!(c||!n)){if(A()&&e.passwordInput.value.trim().length===0){e.setFieldError(e.passwordInput,"Please enter a WiFi password."),e.passwordInput.focus({preventScroll:!0});return}e.clearFieldError(e.passwordInput),c=!0,e.notify(`Connecting to ${n}...`,"info"),e.onStateChange();try{const r=await e.fetchPortalJson("/api/configure",{method:"POST",body:JSON.stringify({ssid:n,password:e.passwordInput.value})});e.notify(r.message||"Connection request sent.","info");for(let d=0;d<e.statusPollAttempts;d++){await e.delayMs(e.statusPollIntervalMs);const g=await e.fetchPortalJson("/api/status");if(k(g),g.connected){e.notify(g.message||"Connected.","success");break}}i||e.notify("Connection in progress...","info")}catch(r){console.error("Connection failed:",r),e.notify(r instanceof Error?r.message:"Connection failed.","error")}finally{c=!1,e.onStateChange()}}}async function w(){if(!(c||l||!i)){l=!0,e.notify("Disconnecting...","info"),e.onStateChange();try{const r=await e.fetchPortalJson("/api/disconnect",{method:"POST"}),d=i,g=o;i=!1,o="",f(),x(),e.notify(r.message||"Disconnected.","success"),e.onConnectionStateChange?.({wasConnected:d,previousNetwork:g,isCurrentlyConnected:i,connectedNetwork:o})}catch(r){console.error("Disconnect failed:",r),e.notify(r instanceof Error?r.message:"Disconnect failed.","error")}finally{l=!1,e.onStateChange()}}}function v(){if(nt()){w();return}if(c||l){e.notify("WiFi status is busy. Please wait.","warning");return}if(!n){e.notify("Select a WiFi network first.","error"),e.networkList.focus({preventScroll:!0});return}if(A()&&e.passwordInput.value.trim().length===0){e.setFieldError(e.passwordInput,"Please enter a WiFi password."),e.notify("Please check if your password is correct.","error"),e.passwordInput.focus({preventScroll:!0});return}e.clearFieldError(e.passwordInput),m()}function T(){e.passwordInput.value?.trim().length&&e.clearFieldError(e.passwordInput),e.onStateChange()}function Bt(r){if(t.length!==0)switch(E(),r.key){case"Tab":break;case"ArrowDown":{r.preventDefault();const d=u<t.length-1?u+1:0;u=d,C(t[d].ssid),e.networkList.scrollOptionIntoView(d);break}case"ArrowUp":{r.preventDefault();const d=u>0?u-1:t.length-1;u=d,C(t[d].ssid),e.networkList.scrollOptionIntoView(d);break}case"Home":{r.preventDefault(),u=0,C(t[0].ssid),e.networkList.scrollOptionIntoView(0);break}case"End":{r.preventDefault();const d=t.length-1;u=d,C(t[d].ssid),e.networkList.scrollOptionIntoView(d);break}case"Enter":case" ":{r.preventDefault(),u>=0&&u<t.length&&C(t[u].ssid);break}}}function nt(){return i&&o.length>0&&n===o}return{applyPortalStatus:k,checkStatus:h,connect:m,disconnect:w,getConnectedNetwork:()=>o,getSelectedNetwork:()=>n,handleConnectAction:v,handleListboxKeyDown:Bt,handlePasswordInput:T,isCheckingStatus:()=>l,isConnecting:()=>c,isCurrentlyConnected:()=>i,isScanning:()=>a,isSelectedConnected:nt,renderNetworkList:x,requiresPassword:A,scanNetworks:s,updateWifiStatusCard:f}}function z(e,t){t().finally(()=>{!e.disabled&&e.offsetParent!==null&&setTimeout(()=>{!e.disabled&&e.offsetParent!==null&&e.focus()},0)})}function wn(e){const{controllers:t,dom:n,helpers:i}=e;function o(){n.wifiSettingsSheet.openBottomSheet(),t.wifiController.scanNetworks()}n.scanBtn.addEventListener("click",()=>{t.wifiController.scanNetworks()}),n.connectBtn.addEventListener("click",()=>{t.wifiController.handleConnectAction()}),n.wifiStatusCard.addEventListener("action",()=>{if(t.wifiController.isCurrentlyConnected()){t.wifiController.disconnect();return}o()}),n.wifiStatusCard.addEventListener("settings",()=>{o()}),n.wifiSettingsCloseBtn.addEventListener("click",()=>{n.wifiSettingsSheet.closeBottomSheet()}),n.passwordInput.addEventListener("input",()=>{t.wifiController.handlePasswordInput()}),n.networkList.addEventListener("keydown",a=>{t.wifiController.handleListboxKeyDown(a)}),n.localAiSaveBtn.addEventListener("click",()=>{z(n.localAiSaveBtn,()=>t.localAiController.saveLocalAiBaseUrl())}),n.localAiResetBtn.addEventListener("click",()=>{z(n.localAiResetBtn,()=>t.localAiController.resetLocalAiBaseUrl())}),n.localAiBaseUrlInput.addEventListener("input",i.updateUi),n.timezoneLocationSaveBtn.addEventListener("click",()=>{if(!n.timezoneSelect.value.trim()){i.setFieldError(n.timezoneSelect,"Select a timezone."),i.setTimezoneLocationNotification("Select a timezone.","error"),n.timezoneSelect.focus({preventScroll:!0});return}i.clearFieldError(n.timezoneSelect),z(n.timezoneLocationSaveBtn,()=>t.timeController.saveTimezoneLocation())}),n.timezoneLocationClearBtn.addEventListener("click",()=>{z(n.timezoneLocationClearBtn,()=>t.timeController.clearTimezoneLocation())}),n.timezoneSelect.addEventListener("change",()=>{n.timezoneSelect.value?.trim().length&&i.clearFieldError(n.timezoneSelect),i.updateUi()}),n.manualDateInput.addEventListener("input",i.updateUi),n.manualTimeInput.addEventListener("input",i.updateUi)}function yn(e){const{controllers:t,dom:n}=e,i=t.wifiController.isScanning()||t.wifiController.isConnecting()||t.wifiController.isCheckingStatus(),o=t.wifiController.isSelectedConnected(),a=t.wifiController.getSelectedNetwork().trim(),l=a.length>0&&!o&&t.wifiController.requiresPassword(),c=o||a.length>0&&(!l||n.passwordInput.value.trim().length>0);n.scanBtn.disabled=i,n.connectBtn.disabled=i||!c,n.wifiStatusCard.actionDisabled=i,n.passwordInput.disabled=t.wifiController.isConnecting(),t.wifiController.isConnecting()?(n.connectBtn.textContent="Connecting...",n.wifiStatusCard.actionLabel="Connecting..."):o?(n.connectBtn.textContent="Disconnect",n.wifiStatusCard.actionLabel="Disconnect"):(n.connectBtn.textContent="Connect",n.wifiStatusCard.actionLabel=t.wifiController.isCurrentlyConnected()?"Disconnect":"Connect");const u=t.timeController.isTimezoneBusy()||t.timeController.isLocationBusy()||t.timeController.isClockBusy();n.timezoneSelect.disabled=u,n.manualDateInput.disabled=u,n.manualTimeInput.disabled=u,n.timezoneLocationSaveBtn.disabled=u||n.timezoneSelect.value.trim().length===0,n.timezoneLocationClearBtn.disabled=u||n.timezoneSelect.value.trim().length===0;const f=t.localAiController.isLocalAiBusy();n.localAiBaseUrlInput.readOnly=f,n.localAiBaseUrlInput.disabled=f,n.localAiSaveBtn.disabled=f||n.localAiBaseUrlInput.value.trim().length===0,n.localAiResetBtn.disabled=f}Zt();Ht();Gt();ne();de();ge();be();Se();_e();let X=null;const b=nn(),Lt=on(b.wifiSettingsNotification),Cn=At(b.localAiCard),_t=At(b.timezoneLocationCard),It=()=>document.createElement("input"),xn=document.createElement("button"),Sn=It(),Tn=It();function L(){yn({controllers:{localAiController:et,timeController:_,wifiController:B},dom:b})}const et=mn({fetchLocalAiModuleJson:Tt,localAiBaseUrlInput:b.localAiBaseUrlInput,isLocalAiModuleActive:()=>!0,notifyLocalAi:Cn,updateButtons:L}),_=pn({applyTalkingClockModuleSettings:()=>{},bedtimeTimeInput:Tn,clearFieldError:Q,clockModeToggle:xn,clockSyncPollAttempts:Ze,clockSyncPollIntervalMs:Ue,fetchTimeRuntimeJson:tn,fetchTimeSettingsJson:Qe,fetchTimezoneListJson:en,focusTalkingClockTimeInput:()=>{},buildTimezoneLabelMap:hn,formatTimezoneLabel:gn,isSwitchChecked:sn,isTalkingClockModuleActive:()=>!1,isTalkingClockModuleBusy:()=>!1,manualDateInput:b.manualDateInput,manualTimeInput:b.manualTimeInput,notifyClockMode:()=>{},notify:_t,onStateChange:L,onTalkingClockBusyChange:()=>{},parseClockTimeInputValue:fn,patchTalkingClockSettings:()=>Promise.resolve({success:!0}),setFieldError:tt,setSwitchChecked:rn,timeRuntimeApi:Ye,timeSettingsApi:je,timezoneSelect:b.timezoneSelect,wakeupTimeInput:Sn}),B=vn({checkIcon:De,clearFieldError:Q,delayMs:ln,fetchPortalJson:Xe,networkList:b.networkList,notify:Lt,onConnectionStateChange:({wasConnected:e,isCurrentlyConnected:t})=>{!e&&t&&_.refreshClockStatusAfterWifiConnect()},onStateChange:L,passwordInput:b.passwordInput,scanPollAttempts:Ke,scanPollIntervalMs:Je,securityIcon:Me,setFieldError:tt,signalIcons:{oneBar:ze,twoBar:qe,threeBar:Ne,fourBar:Pe},statusPollAttempts:Fe,statusPollIntervalMs:Re,svgWithClass:kt,wifiFindIcon:Ve,wifiStatusCard:b.wifiStatusCard});function q(){if(!X)return;const e=document.documentElement,t=e.scrollHeight>window.innerHeight+1,n=window.scrollY+window.innerHeight>=e.scrollHeight-1;X.setVisible(t&&!n)}function An(){an(b.followupLogoEl,Ie,"followup-logo"),b.wifiStatusCard.iconSvg=Be,b.localAiCard.iconSvg=Oe,b.timezoneLocationCard.iconSvg=He,B.renderNetworkList(),document.body&&(X=Nt({mount:document.body,mode:"fixed",position:"bottom",height:"6rem",width:"100vw",strength:1.8,divCount:5,curve:"bezier",exponential:!0,opacity:1,zIndex:20,className:"page-gradual-blur",fallbackColor:"var(--color-bg-selected)"}),q(),window.addEventListener("scroll",q,{passive:!0}),window.addEventListener("resize",q),typeof ResizeObserver<"u"&&new ResizeObserver(()=>{q()}).observe(document.body)),L(),B.updateWifiStatusCard(),Lt(""),wn({controllers:{localAiController:et,timeController:_,wifiController:B},dom:b,helpers:{clearFieldError:Q,setFieldError:tt,setTimezoneLocationNotification:_t,updateUi:L}}),kn(),window.setInterval(()=>{_.fetchTimeRuntimeStatus()},We)}async function kn(){await Promise.allSettled([_.populateTimezoneOptions(),_.fetchTimeSettingsStatus(),B.checkStatus(),En()])}async function En(){try{const e=await Tt("/api/settings/local_ai");et.applyLocalAiSettings(e.settings)}catch(e){console.error("Local AI settings status failed:",e)}finally{L()}}An();
