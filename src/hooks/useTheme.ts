import { useEffect, useState } from 'react';

type Theme = 'dark' | 'light';

export function useTheme() {
	const [theme, setThemeState] = useState<Theme>(() => {
		// Check localStorage first
		const stored = localStorage.getItem('theme') as Theme | null;
		if (stored) return stored;
		
		// Check system preference
		if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
			return 'dark';
		}
		
		return 'dark';
	});

	const setTheme = (newTheme: Theme) => {
		setThemeState(newTheme);
		localStorage.setItem('theme', newTheme);
		
		if (newTheme === 'light') {
			document.body.classList.add('light-mode');
		} else {
			document.body.classList.remove('light-mode');
		}
	};

	// Initialize theme on mount
	useEffect(() => {
		if (theme === 'light') {
			document.body.classList.add('light-mode');
		} else {
			document.body.classList.remove('light-mode');
		}
	}, []);

	const toggleTheme = () => {
		setTheme(theme === 'dark' ? 'light' : 'dark');
	};

	return { theme, setTheme, toggleTheme };
}
