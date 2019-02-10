object ProcessGuardService: TProcessGuardService
  OldCreateOrder = False
  DisplayName = 'ProcessGuardService'
  AfterInstall = ServiceAfterInstall
  OnContinue = ServiceContinue
  OnPause = ServicePause
  OnShutdown = ServiceShutdown
  OnStart = ServiceStart
  OnStop = ServiceStop
  Height = 150
  Width = 215
  object ProcessTimer: TTimer
    Enabled = False
    Interval = 10000
    OnTimer = ProcessTimerTimer
    Left = 96
    Top = 16
  end
end
