function receiver_parameters = load_receiver_parameters()
    receiver_parameters.f0 = 1e6; % frequency distance between measurementes
    receiver_parameters.M1 = 4; % number of frequency measurements on each channel observation
    receiver_parameters.M2 = 4; % number of horizontal elements in URA antenna array
    receiver_parameters.M3 = 4; % number of vertical elements in URA antenna array
    receiver_parameters.d = 40e-3; % elements distance in URA antenna array
    receiver_parameters.noise_power = 1e-12; % awg noise power in W
    receiver_parameters.lam = physconst('LightSpeed')/2.45e9; % central wavelength
    receiver_parameters.height = 3; % in meters
end