import { ActuatorKey, ActuatorState } from '../types';

interface Props {
	mode: 'auto' | 'manual';
	onModeChange: (mode: 'auto' | 'manual') => void;
	actuators: Record<ActuatorKey, ActuatorState>;
	onToggle: (key: ActuatorKey, next: boolean) => void;
	onAll: (next: boolean) => void;
}

function Toggle({ on, onClick }: { on: boolean; onClick: () => void }) {
	return (
		<div className={`switch ${on ? 'on' : ''}`} onClick={onClick} role="switch" aria-checked={on}>
			<div className="knob" />
		</div>
	);
}

export default function ActuatorPanel({ mode, onModeChange, actuators, onToggle, onAll }: Props) {
	const isAuto = mode === 'auto';
	return (
		<div className="card">
			<div className="row" style={{ marginBottom: 12 }}>
				<div className="title" style={{ fontSize: 18, margin: 0 }}>Aktuator</div>
				<div className="spacer" />
				<div className="badge">{isAuto ? 'Mode Otomatis' : 'Mode Manual'}</div>
			</div>
			<div className="row" style={{ marginBottom: 12, gap: 16 }}>
				<button className="btn" onClick={() => onModeChange('auto')} disabled={isAuto}>Otomatis</button>
				<button className="btn" onClick={() => onModeChange('manual')} disabled={!isAuto}>Manual</button>
				<div className="spacer" />
				<button className="btn ok" onClick={() => onAll(true)} disabled={isAuto}>Semua ON</button>
				<button className="btn danger" onClick={() => onAll(false)} disabled={isAuto}>Semua OFF</button>
			</div>
			<div className="actuator-grid">
				{(['lamp', 'fan', 'pump'] as ActuatorKey[]).map((k) => {
					const a = actuators[k];
					return (
						<div key={k} className="card actuator-item">
							<div className="row">
								<div className="title" style={{ fontSize: 16, margin: 0 }}>{a.name}</div>
								<div className="spacer" />
								<div className="badge" style={{ color: a.isOn ? 'var(--ok)' : 'var(--muted)' }}>{a.isOn ? 'Nyala' : 'Mati'}</div>
							</div>
							<div className="row" style={{ marginTop: 10 }}>
								<Toggle on={a.isOn} onClick={() => !isAuto && onToggle(k, !a.isOn)} />
								<div className="subtitle" style={{ margin: 0 }}>{isAuto ? 'Dikendalikan otomatis' : 'Tekan untuk ubah'}</div>
							</div>
						</div>
					);
				})}
			</div>
		</div>
	);
}

