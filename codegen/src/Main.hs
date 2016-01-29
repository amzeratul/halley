{-
   Copyright 2016 Rodrigo Braz Monteiro

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
-}

module Main where

import System.Environment
import System.Directory
import qualified Filesystem.Path.CurrentOS as Path

import Control.Monad
import Data.List
import Data.String.Utils

import Halley.Parser
import Halley.AST
import qualified Halley.SemanticAnalysis as Semantics
import qualified Halley.CodeGenCpp as CodeGenCpp
import qualified Halley.CodeGen as CodeGen

import Text.ParserCombinators.Parsec


---------------------------


main = do
    args <- getArgs
    let inDir = args !! 0
    let outDir = args !! 1
    dirContents <- getDirectoryContents inDir
    let inFiles = map (\x -> inDir ++ "/" ++ x) (filter (endswith ".txt") dirContents)
    parseStage outDir inFiles

parseStage :: String -> [String] -> IO ()
parseStage outDir files = do
    parseResult <- parseFiles files
    case parseResult of
        Left error -> putStrLn $ show error
        Right defs -> semanticStage outDir defs

semanticStage :: String -> [GenDefinition] -> IO ()
semanticStage outDir defs = do
    case Semantics.semanticAnalysis defs of
        Left error -> putStrLn error
        Right dataToGen -> do
            putStrLn("----------\nComponents:\n")
            mapM_ (\x -> putStrLn $ show x ++ "\n") $ Semantics.components dataToGen
            putStrLn("----------\nSystems:\n")
            mapM_ (\x -> putStrLn $ show x ++ "\n") $ Semantics.systems dataToGen
            codeGenStage outDir dataToGen

codeGenStage :: String -> Semantics.CodeGenData -> IO ()
codeGenStage outDir dataToGen = do
    putStrLn("----------\nData generated:\n")
    mapM_ (\x -> writeAndPrint outDir True x) results
    where
        results = CodeGenCpp.generateCodeCpp dataToGen

writeAndPrint :: FilePath -> Bool -> CodeGen.GeneratedSource -> IO()
writeAndPrint fileRoot doPrint file = do
    if doPrint then do
        putStrLn "============="
        putStrLn $ CodeGen.filename file
        putStrLn "-------------"
        putStrLn $ CodeGen.code file
        putStrLn ""
    else
        return ()
    createDirectoryIfMissing True (Path.encodeString $ Path.directory $ Path.decodeString path)
    writeFile path (CodeGen.code file)
    where
        path = fileRoot ++ "/" ++ (CodeGen.filename file)

parseFiles fs = do
    fmap (foldEither) (mapM (parse) fs)
    where
        parse f = fmap (parseFile f) (readFile f)

foldEither vs = foldl' (\a b -> (++) <$> a <*> b) (Right []) vs
