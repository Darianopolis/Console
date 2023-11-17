if Project "console" then
    Compile "src/**"
    Include "src"
    Artifact { "out/main", type = "Console" }
end

if Project "console-test" then
    Compile "test.cpp"
    Artifact { "out/test", type = "Console" }
end