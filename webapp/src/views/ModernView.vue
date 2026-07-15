<template>
    <main class="modern-dashboard">
        <section class="modern-shell">
            <header class="modern-hero">
                <div>
                    <p class="eyebrow"><span>Open</span><span>DTU</span></p>
                    <h1>LiveView</h1>
                    <div class="status-row">
                        <span class="status-pill" :class="{ live: isWebsocketConnected }">
                            {{ isWebsocketConnected ? 'Live' : 'Offline' }}
                        </span>
                        <span>{{ inverterName }}</span>
                        <span>S/N {{ inverterSerial }}</span>
                        <span>Data age {{ dataAgeText }}</span>
                        <span>{{ currentLimitText }}</span>
                    </div>
                </div>
                <router-link class="classic-link" to="/">Classic View</router-link>
            </header>

            <section class="overview-grid">
                <article class="power-card">
                    <div class="gauge-wrap">
                        <div class="gauge">
                            <svg class="gauge-arc" viewBox="0 0 300 170" aria-hidden="true">
                                <defs>
                                    <linearGradient id="powerGaugeGradient" x1="35" y1="150" x2="265" y2="150" gradientUnits="userSpaceOnUse">
                                        <stop offset="0%" stop-color="#1fa8ff" />
                                        <stop offset="100%" stop-color="#19f5a7" />
                                    </linearGradient>
                                </defs>
                                <path class="gauge-track" pathLength="100" d="M 35 150 A 115 115 0 0 1 265 150" />
                                <path
                                    class="gauge-progress"
                                    pathLength="100"
                                    d="M 35 150 A 115 115 0 0 1 265 150"
                                    :style="{ strokeDasharray: `${powerGaugePercent} 100` }"
                                />
                            </svg>
                            <div class="gauge-inner">
                                <span class="gauge-label">Current Power</span>
                                <strong>{{ powerValue }}</strong>
                                <span>{{ powerUnit }}</span>
                            </div>
                        </div>
                        <div class="gauge-scale">
                            <span>0 W</span>
                            <span>800 W</span>
                            <span>1600 W</span>
                        </div>
                    </div>
                </article>

                <article class="metric-card accent-amber">
                    <BIconSun class="metric-icon" />
                    <span>Today</span>
                    <strong>{{ yieldDayKWh }}</strong>
                    <small>kWh</small>
                </article>

                <article class="metric-card accent-purple">
                    <BIconCalculator class="metric-icon" />
                    <span>Total</span>
                    <strong>{{ yieldTotalKWh }}</strong>
                    <small>kWh</small>
                </article>

                <article class="metric-card accent-green">
                    <BIconFlower1 class="metric-icon" />
                    <span>CO2</span>
                    <strong>{{ co2Kg }}</strong>
                    <small>kg saved</small>
                </article>
            </section>

            <section class="content-grid">
                <article class="panel-card">
                    <div class="section-head">
                        <div>
                            <p>PV Strings</p>
                            <h2>Power per Panel</h2>
                        </div>
                        <span>{{ dcPowerTotal }}</span>
                    </div>
                    <div class="bar-chart">
                        <div v-for="panel in panels" :key="panel.label" class="bar-slot">
                            <span>{{ panel.powerText }}</span>
                            <div class="bar-track">
                                <div class="bar-fill" :style="{ height: panel.height + '%' }"></div>
                            </div>
                            <strong>{{ panel.label }}</strong>
                        </div>
                    </div>
                </article>

                <article class="status-card">
                    <div class="section-head">
                        <div>
                            <p>Inverter</p>
                            <h2>{{ inverterName }}</h2>
                        </div>
                        <span :class="{ ok: firstInverter?.reachable }">
                            {{ firstInverter?.reachable ? 'Online' : 'Offline' }}
                        </span>
                    </div>
                    <div class="status-grid">
                        <div class="mini-tile">
                            <BIconLightningCharge />
                            <span>AC Power</span>
                            <strong>{{ acPower }}</strong>
                        </div>
                        <div class="mini-tile">
                            <BIconThermometerHalf />
                            <span>Temperature</span>
                            <strong>{{ temperature }}</strong>
                        </div>
                        <div class="mini-tile">
                            <BIconActivity />
                            <span>Efficiency</span>
                            <strong>{{ efficiency }}</strong>
                        </div>
                        <div class="mini-tile">
                            <BIconReception4 />
                            <span>RSSI</span>
                            <strong>{{ rssi }}</strong>
                        </div>
                    </div>
                </article>
            </section>

            <section class="detail-grid">
                <article class="detail-card" v-for="item in detailItems" :key="item.label">
                    <span>{{ item.label }}</span>
                    <strong>{{ item.value }}</strong>
                </article>
            </section>
        </section>
    </main>
</template>

<script lang="ts">
import type { Inverter, LiveData, ValueObject } from '@/types/LiveDataStatus';
import { authHeader, authUrl, handleResponse } from '@/utils/authentication';
import WebSocketService from '@/utils/websocketService';
import {
    BIconActivity,
    BIconCalculator,
    BIconFlower1,
    BIconLightningCharge,
    BIconReception4,
    BIconSun,
    BIconThermometerHalf,
} from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

type PanelInfo = {
    label: string;
    height: number;
    powerText: string;
};

export default defineComponent({
    components: {
        BIconActivity,
        BIconCalculator,
        BIconFlower1,
        BIconLightningCharge,
        BIconReception4,
        BIconSun,
        BIconThermometerHalf,
    },
    data() {
        return {
            socket: null as WebSocketService | null,
            liveData: null as LiveData | null,
            isWebsocketConnected: false,
        };
    },
    created() {
        this.loadData();
        this.initSocket();
    },
    unmounted() {
        this.socket?.close();
    },
    computed: {
        totalPower(): number {
            return this.liveData?.total?.Power?.v ?? 0;
        },
        powerValue(): string {
            return this.totalPower >= 1000 ? (this.totalPower / 1000).toFixed(1) : this.totalPower.toFixed(0);
        },
        powerUnit(): string {
            return this.totalPower >= 1000 ? 'kW' : 'W';
        },
        powerGaugePercent(): string {
            const ratio = Math.max(0, Math.min(this.totalPower / 1600, 1));
            return (ratio * 100).toFixed(1);
        },
        yieldDayKWh(): string {
            return ((this.liveData?.total?.YieldDay?.v ?? 0) / 1000).toFixed(2);
        },
        yieldTotalKWh(): string {
            return (this.liveData?.total?.YieldTotal?.v ?? 0).toFixed(2);
        },
        co2Kg(): string {
            return ((this.liveData?.total?.YieldTotal?.v ?? 0) * 0.6).toFixed(1);
        },
        inverters(): Inverter[] {
            return [...(this.liveData?.inverters ?? [])].sort((a, b) => a.order - b.order);
        },
        firstInverter(): Inverter | undefined {
            return this.inverters[0];
        },
        inverterName(): string {
            return this.firstInverter?.name ?? 'MoschBalkonPV';
        },
        inverterSerial(): string {
            return this.firstInverter?.serial ?? '--';
        },
        dataAgeText(): string {
            const ageMs = this.firstInverter?.data_age_ms ?? 0;
            return `${Math.round(ageMs / 1000)}s`;
        },
        panels(): PanelInfo[] {
            const channels = this.firstInverter?.DC ?? [];
            const powers = [0, 1, 2, 3].map((idx) => channels[idx]?.Power?.v ?? 0);
            const max = Math.max(1, ...powers);
            return powers.map((power, idx) => ({
                label: `P${idx + 1}`,
                height: Math.max(4, Math.round((power / max) * 100)),
                powerText: `${power.toFixed(0)} W`,
            }));
        },
        dcPowerTotal(): string {
            const watts = this.panels.reduce((sum, panel) => sum + Number.parseFloat(panel.powerText), 0);
            return `${watts.toFixed(0)} W DC`;
        },
        acPower(): string {
            return this.formatValue(this.firstInverter?.AC?.[0]?.Power);
        },
        temperature(): string {
            return this.formatValue(this.firstInverter?.INV?.[0]?.Temperature);
        },
        efficiency(): string {
            return this.formatValue(this.firstInverter?.INV?.[0]?.Efficiency);
        },
        rssi(): string {
            const value = this.firstInverter?.radio_stats?.rssi;
            return value === undefined ? '--' : `${value} dBm`;
        },
        detailItems(): { label: string; value: string }[] {
            return [
                { label: 'AC Voltage', value: this.formatValue(this.firstInverter?.AC?.[0]?.Voltage) },
                { label: 'AC Current', value: this.formatValue(this.firstInverter?.AC?.[0]?.Current) },
                { label: 'Frequency', value: this.formatValue(this.firstInverter?.AC?.[0]?.Frequency) },
                { label: 'Power Factor', value: this.formatValue(this.firstInverter?.AC?.[0]?.PowerFactor) },
                { label: 'Limit', value: this.limitText },
                { label: 'Events', value: `${this.firstInverter?.events ?? 0}` },
            ];
        },
        limitText(): string {
            const inv = this.firstInverter;
            if (!inv) {
                return '--';
            }
            return `${inv.limit_absolute.toFixed(0)} W / ${inv.limit_relative.toFixed(1)}%`;
        },
        currentLimitText(): string {
            return `Current Limit: ${this.limitText}`;
        },
    },
    methods: {
        formatValue(value?: ValueObject): string {
            if (!value) {
                return '--';
            }
            return `${value.v.toFixed(value.d ?? 1)} ${value.u}`;
        },
        loadData() {
            fetch('/api/livedata/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data: LiveData) => {
                    this.liveData = data;
                });
        },
        initSocket() {
            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === 'https:' ? 'wss' : 'ws'}://${authString}${host}/livedata`;

            this.socket = new WebSocketService(webSocketUrl, {
                onMessage: this.handleMessage,
                onOpen: () => {
                    this.isWebsocketConnected = true;
                },
                onClose: () => {
                    this.isWebsocketConnected = false;
                },
            });

            this.socket.connect();
        },
        handleMessage(event: MessageEvent) {
            if (!event.data || event.data === '{}') {
                this.socket?.close();
                this.initSocket();
                return;
            }

            const newData = JSON.parse(event.data) as LiveData;
            if (!this.liveData) {
                this.liveData = newData;
                return;
            }

            Object.assign(this.liveData.total, newData.total);
            Object.assign(this.liveData.hints, newData.hints);

            const incoming = newData.inverters?.[0];
            if (!incoming) {
                return;
            }
            const idx = this.liveData.inverters.findIndex((inv) => inv.serial === incoming.serial);
            if (idx >= 0) {
                const existing = this.liveData.inverters[idx];
                if (existing) {
                    Object.assign(existing, incoming);
                }
            } else {
                this.liveData.inverters.push(incoming);
            }
        },
    },
});
</script>

<style scoped>
.modern-dashboard {
    min-height: calc(100vh - 4.5rem);
    margin-top: -0.5rem;
    background:
        radial-gradient(circle at 18% 8%, rgba(23, 165, 255, 0.16), transparent 30rem),
        radial-gradient(circle at 82% 0%, rgba(25, 245, 167, 0.12), transparent 28rem),
        #070a0f;
    color: #eef6ff;
}

.modern-shell {
    max-width: 1500px;
    margin: 0 auto;
    padding: 1.4rem;
}

.modern-hero,
.overview-grid,
.content-grid,
.detail-grid {
    display: grid;
    gap: 1rem;
}

.modern-hero {
    grid-template-columns: 1fr auto;
    align-items: center;
    margin-bottom: 1rem;
}

.eyebrow {
    margin: 0;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
}

.eyebrow span:first-child {
    color: #eef6ff;
}

.eyebrow span:last-child {
    color: #17a5ff;
}

h1,
h2,
p {
    margin: 0;
}

h1 {
    font-size: clamp(2.2rem, 4vw, 4.6rem);
    line-height: 0.95;
}

.status-row {
    display: flex;
    flex-wrap: wrap;
    gap: 0.6rem;
    margin-top: 0.75rem;
    color: #9da9b8;
}

.status-row span,
.classic-link {
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 999px;
    padding: 0.42rem 0.72rem;
    background: rgba(255, 255, 255, 0.045);
}

.status-pill.live {
    color: #101513;
    background: #19f5a7;
}

.classic-link {
    color: #dce8f7;
    text-decoration: none;
}

.overview-grid {
    grid-template-columns: minmax(20rem, 1.6fr) repeat(3, minmax(12rem, 1fr));
}

.content-grid {
    grid-template-columns: minmax(22rem, 1.35fr) minmax(20rem, 1fr);
    margin-top: 1rem;
}

.detail-grid {
    grid-template-columns: repeat(auto-fit, minmax(11.5rem, 1fr));
    margin-top: 1rem;
}

.power-card,
.metric-card,
.panel-card,
.status-card,
.detail-card {
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    background: linear-gradient(180deg, rgba(18, 24, 32, 0.92), rgba(10, 13, 18, 0.96));
    box-shadow: 0 1rem 3rem rgba(0, 0, 0, 0.25);
}

.power-card {
    min-height: 21rem;
    display: grid;
    place-items: center;
}

.gauge-wrap {
    width: min(28rem, 88%);
}

.gauge {
    position: relative;
    aspect-ratio: 2 / 1;
    overflow: visible;
}

.gauge-arc {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
}

.gauge-track,
.gauge-progress {
    fill: none;
    stroke-linecap: butt;
    stroke-width: 22;
}

.gauge-track {
    stroke: #202833;
}

.gauge-progress {
    stroke: url('#powerGaugeGradient');
}

.gauge-inner {
    position: absolute;
    z-index: 1;
    inset: 36% 12% 0;
    display: grid;
    place-items: center;
}

.gauge-label,
.metric-card span,
.detail-card span,
.mini-tile span,
.section-head p {
    color: #9da9b8;
    font-size: 0.82rem;
    text-transform: uppercase;
}

.gauge strong {
    font-size: clamp(4rem, 9vw, 7rem);
    line-height: 0.86;
}

.gauge-inner span:last-child {
    color: #17a5ff;
    font-size: 1.6rem;
    font-weight: 700;
}

.gauge-scale {
    display: flex;
    justify-content: space-between;
    color: #9da9b8;
    margin-top: 0.35rem;
}

.metric-card {
    min-height: 21rem;
    padding: 1.25rem;
    display: grid;
    align-content: center;
    justify-items: center;
    text-align: center;
}

.metric-icon {
    font-size: 3.2rem;
    margin-bottom: 1.5rem;
}

.metric-card strong {
    font-size: clamp(3.2rem, 5vw, 5.4rem);
    line-height: 1;
}

.metric-card small {
    color: #9da9b8;
    font-size: 1rem;
}

.accent-amber .metric-icon,
.accent-amber span {
    color: #ffd21f;
}

.accent-purple .metric-icon,
.accent-purple span {
    color: #a970ff;
}

.accent-green .metric-icon,
.accent-green span {
    color: #19f5a7;
}

.panel-card,
.status-card {
    padding: 1.2rem;
}

.section-head {
    display: flex;
    align-items: start;
    justify-content: space-between;
    gap: 1rem;
    margin-bottom: 1.25rem;
}

.section-head h2 {
    font-size: clamp(1.35rem, 2vw, 2rem);
}

.section-head > span {
    color: #17a5ff;
    font-weight: 700;
}

.section-head > span.ok {
    color: #19f5a7;
}

.bar-chart {
    min-height: 18rem;
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 1rem;
    align-items: end;
}

.bar-slot {
    display: grid;
    grid-template-rows: auto 1fr auto;
    gap: 0.5rem;
    height: 100%;
    text-align: center;
}

.bar-slot span {
    color: #dce8f7;
    font-weight: 700;
}

.bar-track {
    align-self: end;
    height: 14rem;
    border-radius: 6px;
    background: rgba(255, 255, 255, 0.06);
    overflow: hidden;
    display: flex;
    align-items: end;
}

.bar-fill {
    width: 100%;
    min-height: 4%;
    background: linear-gradient(180deg, #19f5a7, #17a5ff);
}

.bar-slot strong {
    color: #9da9b8;
}

.status-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 0.8rem;
}

.mini-tile,
.detail-card {
    border: 1px solid rgba(255, 255, 255, 0.09);
    border-radius: 8px;
    background: rgba(255, 255, 255, 0.045);
}

.mini-tile {
    padding: 1rem;
    display: grid;
    gap: 0.35rem;
}

.mini-tile svg {
    color: #17a5ff;
    font-size: 1.55rem;
}

.mini-tile strong,
.detail-card strong {
    font-size: 1.4rem;
}

.detail-card {
    padding: 1rem;
    display: grid;
    align-content: center;
    justify-items: center;
    gap: 0.45rem;
    min-height: 5rem;
    text-align: center;
}

.detail-card span {
    overflow-wrap: anywhere;
}

.detail-card strong {
    white-space: nowrap;
}

@media (max-width: 1100px) {
    .overview-grid,
    .content-grid,
    .detail-grid {
        grid-template-columns: repeat(2, minmax(0, 1fr));
    }
}

@media (max-width: 700px) {
    .modern-hero,
    .overview-grid,
    .content-grid,
    .detail-grid {
        grid-template-columns: 1fr;
    }
}
</style>
